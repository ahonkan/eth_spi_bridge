/*************************************************************************
*
*              Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILENAME
*
*       dvi_common.c
*
*   COMPONENT
*
*       DV - Device Manager
*
*   DESCRIPTION
*
*       This file contains the common initialization routine for this
*       component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       nu_os_kern_devmgr_init              Initializes the DM component
*       DVI_Device_Discovery_Task_Entry     Entry function for device
*                                           discovery task.
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       event_group.h
*       device_manager.h
*       proc_extern.h
*       event_notification.h
*
*************************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "services/nu_services.h"
#include        "os/kernel/plus/core/inc/event_group.h"
#include        "os/kernel/devmgr/inc/device_manager.h"

#if (CFG_NU_OS_KERN_DEVMGR_EXPORT_SYMBOLS == NU_TRUE)

#include        "kernel/proc_extern.h"

#endif /* CFG_NU_OS_KERN_DEVMGR_EXPORT_SYMBOLS */

#if (CFG_NU_OS_KERN_PLUS_SUPPLEMENT_EVT_NOTIFY == NU_TRUE)
#include "os/kernel/plus/supplement/inc/event_notification.h"
/* Notification Registry */
extern EN_REGISTRY *END_Registry;
#endif /* CFG_NU_OS_KERN_PLUS_SUPPLEMENT_EVT_NOTIFY == NU_TRUE */

/* Define external inner-component global data references.  */

extern UNSIGNED         DVD_Initialized;
extern INT              DVD_Max_Dev_Id_Cnt;
extern DV_DEV_REGISTRY  *DVD_Dev_Registry;
extern NU_EVENT_GROUP   DVD_Reg_Change_Event;

#if (DV_DEV_DISCOVERY_TASK_ENB == NU_TRUE)
/* System memory pool. */
extern NU_MEMORY_POOL   System_Memory;

/* Control block for device discovery task. */
extern NU_TASK          DVD_Dev_Discovery_Task;

/* Semaphore for the protection of listener array. */
extern NU_SEMAPHORE     DVD_Dev_Reg_Listener_Semaphore;

/* External data structure and function. */
extern DV_DEV_LISTENER *DVD_Dev_Reg_Listener_Array[DV_MAX_DEVICE_LISTENERS];

/* Entry function header for device discovery task. */
static VOID DVI_Device_Discovery_Task_Entry(UNSIGNED argc, VOID *argv);
#endif /* DV_DEV_DISCOVERY_TASK_ENB == NU_TRUE */

/*************************************************************************
*
*   FUNCTION
*
*       nu_os_kern_devmgr_init
*
*   DESCRIPTION
*
*       Initializes the DM Component
*
*   CALLED BY
*
*       Runlevel Init System
*
*   CALLS
*
*       NU_Create_Event_Group
*
*   INPUTS
*
*       CHAR          *key                  - Key
*       INT           startstop             - Start or stop flag
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID nu_os_kern_devmgr_init (const CHAR * key, INT startstop)
{
    INT             i;
    NU_MEMORY_POOL  *sys_pool;
    STATUS          status;

    /* Suppress warnings */
    NU_UNUSED_PARAM(key);
    NU_UNUSED_PARAM(startstop);

#if (CFG_NU_OS_KERN_DEVMGR_EXPORT_SYMBOLS == NU_TRUE)

    /* Keep symbols for nu.os.kern.devmgr */
    NU_KEEP_COMPONENT_SYMBOLS(NU_OS_KERN_DEVMGR);

#endif /* CFG_NU_OS_KERN_DEVMGR_EXPORT_SYMBOLS */

    /* Only do initialization for start mode */
    if (startstop == RUNLEVEL_START)
    {
        /* Get the number of device ids required */
        status = REG_Get_UINT32_Value(key, "/max_dev_id_cnt", (UINT32 *)&DVD_Max_Dev_Id_Cnt);
    
        if (status == NU_SUCCESS)
        {
            /* Get the System Memory pool pointer */
            status = NU_System_Memory_Get(&sys_pool, NU_NULL);
    
            if (status == NU_SUCCESS)
            {
                /* Allocate memory for the Device Registry */
                status = NU_Allocate_Memory(sys_pool, (VOID *)&DVD_Dev_Registry,
                                            (sizeof(DV_DEV_REGISTRY) * DVD_Max_Dev_Id_Cnt),
                                            NU_NO_SUSPEND);
    
                if (status == NU_SUCCESS)
                {
                    /* Clear the allocated memory */
                    (VOID)memset((VOID *)DVD_Dev_Registry, 0,
                                 (sizeof(DV_DEV_REGISTRY) * DVD_Max_Dev_Id_Cnt));
    
                    /* Initialize the session id reuse count */
                    for (i = 0; i < (INT)DVD_Max_Dev_Id_Cnt; i++)
                    {
                        DVD_Dev_Registry[i].reuse_cnt = 1;
                    }
    
                    /* Create an event group for the DM Registry Change system to use */
                    status = NU_Create_Event_Group(&DVD_Reg_Change_Event, "DVRGEVT");
    
                    if (status != NU_SUCCESS)
                    {
                        /* Deallocate the Device Registry memory */
                        (VOID)NU_Deallocate_Memory((VOID *)DVD_Dev_Registry);
                    }
                }
            }
        }

#if (DV_DEV_DISCOVERY_TASK_ENB == NU_TRUE)
        if(status == NU_SUCCESS)
        {
            /* Declare a temporary void pointer for memory allocation. */
            VOID  *pointer;
    
            /* Create semaphore to protect against simultaneous access of listener array
             * for write operation.
             */
            status = NU_Create_Semaphore(&DVD_Dev_Reg_Listener_Semaphore, "DevList",
                                         (UNSIGNED)1,(OPTION)NU_FIFO);
            if (status == NU_SUCCESS)
            {
                /* Allocate memory for device discovery task stack. */
                status = NU_Allocate_Memory(&System_Memory, &pointer,
                                            DV_DEVICE_DISCOVERY_TASK_STACK_SIZE, NU_NO_SUSPEND);
                if(status == NU_SUCCESS)
                {
                    /* Create device discovery task.  This task will keep waiting for devices to
                     * register and will invoke callbacks upon finding a valid device/listener pair.
                     */
                    status = NU_Create_Task(&DVD_Dev_Discovery_Task, "DevDisc", DVI_Device_Discovery_Task_Entry,
                                            0, NU_NULL, pointer, DV_DEVICE_DISCOVERY_TASK_STACK_SIZE,
                                            DV_DEVICE_DISCOVERY_TASK_PRIORITY, DV_DEVICE_DISCOVERY_TASK_TIMESLICE,
                                            NU_NO_PREEMPT, NU_START);
    
                    /* In case task is not created successfully, reclaim the previously allocated
                     * task stack memory .
                     */
                    if(status != NU_SUCCESS)
                    {
                        NU_Deallocate_Memory(pointer);
                    }
                }
            }
        }
#endif /* DV_DEV_DISCOVERY_TASK_ENB == NU_TRUE */

        /* If we successfully created the event group and (optional) device discovery task. */
        if (status == NU_SUCCESS)
        {
            /* Set the DM initialized flag */
            DVD_Initialized = DV_INITIALIZED;
        }

#if (CFG_NU_OS_KERN_PLUS_SUPPLEMENT_EVT_NOTIFY == NU_TRUE)
        if (status == NU_SUCCESS)
        {
            /* Get the System Memory pool pointer */
            status = NU_System_Memory_Get(&sys_pool, NU_NULL);
    
            if (status == NU_SUCCESS)
            {
                /* Allocate memory for the Notification Registry */
                status = NU_Allocate_Memory(sys_pool, (VOID *)&END_Registry,
                                            (sizeof(EN_REGISTRY) * DVD_Max_Dev_Id_Cnt),
                                            NU_NO_SUSPEND);
    
                if (status == NU_SUCCESS)
                {
                    /* Clear the allocated memory */
                    (VOID)memset((VOID *)END_Registry, 0,
                                 (sizeof(EN_REGISTRY) * DVD_Max_Dev_Id_Cnt));
                }
            }
        }
#endif /* CFG_NU_OS_KERN_PLUS_SUPPLEMENT_EVT_NOTIFY == NU_TRUE */
    }
}

#if (DV_DEV_DISCOVERY_TASK_ENB == NU_TRUE)
/*************************************************************************
*
*   FUNCTION
*
*       DVI_Device_Discovery_Task_Entry
*
*   DESCRIPTION
*
*       It is the entry function for device discovery task. It keeps on
*       waiting for any device register/unregister or scan registry event.
*       Upon receiving a valid event it searches device registry to find what
*       change has occurred against particular device label(list), mentioned by
*       listener. Register or unregister callbacks are invoked for all relevant
*       device IDs.
*
*   CALLED BY
*
*       Device discovery task (entry function).
*
*   CALLS
*
*       NU_Retrieve_Events
*       DVC_Reg_Change_Search
*
*   INPUTS
*
*       argc                                Number of arguments, equal to 0.
*       argv                                Value of arguments, equal to null.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID DVI_Device_Discovery_Task_Entry(UNSIGNED argc, VOID *argv)
{
    INT             new_reg_count;
    INT             new_unreg_count;
    INT             dev_reg_list_count;
    INT             dev_unreg_list_count;
    UINT8           index;
    STATUS          status;
    UNSIGNED        event_group;
    DV_DEV_ID       dev_reg_list[DV_DISCOVERY_TASK_MAX_ID_CNT];
    DV_DEV_ID       dev_unreg_list[DV_DISCOVERY_TASK_MAX_ID_CNT];
    DV_DEV_LISTENER *curr_listener_ptr;
    DV_APP_REGISTRY_CHANGE  device_state_info;

    /* Suppress warnings */
    NU_UNUSED_PARAM(argc);
    NU_UNUSED_PARAM(argv);

    /* Initially clear device state info control block. */
    memset(&device_state_info, 0x00, sizeof(device_state_info));

    /* This task will continue to run forever. */
    while(1)
    {
        /* Wait for a device registry change or a new listener addition. */
        status = NU_Retrieve_Events(&DVD_Reg_Change_Event, (DV_REG_CHANGE_BIT_MASK | DV_DEV_SCAN_REGISTRY_BIT),
                                     (OPTION)NU_OR_CONSUME, &event_group, NU_SUSPEND);

        if(status == NU_SUCCESS)
        {
            /* If there is any change in either device registry or listener array, scan registry to find any
             * available device event for listeners.
             */
            for(index = 0; index < DV_MAX_DEVICE_LISTENERS; index++)
            {
                if(DVD_Dev_Reg_Listener_Array[index] != NU_NULL)
                {
                    /* Get a temporary local copy. */
                    curr_listener_ptr = DVD_Dev_Reg_Listener_Array[index];

                    /* Initially device register/unregister count is zero. */
                    dev_reg_list_count      = 0;
                    dev_unreg_list_count    = 0;

                    /* Clear device register/unregister list. */
                    memset(dev_reg_list, 0x00, sizeof(dev_reg_list));
                    memset(dev_unreg_list, 0x00, sizeof(dev_unreg_list));

                    /* Populate the device state information structure to send to the device manager. */
                    device_state_info.dev_label_list_ptr    = curr_listener_ptr->dev_label_list;
                    device_state_info.dev_label_cnt         = curr_listener_ptr->dev_label_cnt;
                    device_state_info.known_id_list_ptr     = curr_listener_ptr->known_id_list;
                    device_state_info.known_id_cnt_ptr      = &curr_listener_ptr->known_id_cnt;
                    device_state_info.max_id_cnt            = DV_DISCOVERY_TASK_MAX_ID_CNT;
                    device_state_info.reg_id_list_ptr       = dev_reg_list;
                    device_state_info.reg_id_cnt_ptr        = &dev_reg_list_count;
                    device_state_info.unreg_id_list_ptr     = dev_unreg_list;
                    device_state_info.unreg_id_cnt_ptr      = &dev_unreg_list_count;

                    /* Get current state of the devices we are looking for. */
                    status = DVC_Reg_Change_Search(&device_state_info);

                    if(status == NU_SUCCESS)
                    {
                        /* Loop through the newly added devices. */
                        for (new_reg_count = 0; new_reg_count < *device_state_info.reg_id_cnt_ptr; new_reg_count++)
                        {
                            if((curr_listener_ptr->device_register_callback) != NU_NULL)
                            {
                                curr_listener_ptr->device_register_callback(dev_reg_list[new_reg_count], curr_listener_ptr->context);
                            }
                        }

                        /* Loop through the newly removed devices. */
                        for (new_unreg_count = 0; new_unreg_count < *device_state_info.unreg_id_cnt_ptr; new_unreg_count++)
                        {
                            if((curr_listener_ptr->device_unregister_callback) != NU_NULL)
                            {
                                curr_listener_ptr->device_unregister_callback(dev_unreg_list[new_unreg_count], curr_listener_ptr->context);
                            }
                        }
                    }
                }
            }
        }
    }
}

#endif /* DV_DEV_DISCOVERY_TASK_ENB == NU_TRUE */


/***********************************************************************
*
*           Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
************************************************************************

*************************************************************************
*
* FILE NAME
*
*    nu_usbh_stack_ext.c
*
* COMPONENT
*
*     Nucleus USB Host Stack
*
* DESCRIPTION
*     This file contains the external Interfaces exposed by Nucleus USB
*     Host Stack component of Nucleus USB Host software.
*
* DATA STRUCTURES
*     None
*
* FUNCTIONS
*      NU_USBH_STACK_Create                  creates a host stack.
*     _NU_USBH_STACK_Create                  protected constructor.
*     _NU_USBH_STACK_Add_Hw                  adds a HW controller to host
*                                            stack.
*     _NU_USBH_STACK_Remove_Hw               removes a HW controller from
*                                            stack.
*     _NU_USBH_STACK_Delete                  deletes an instance of host
*                                            stack.
*     _NU_USBH_STACK_Deregister_Drvr         deregisters a driver from stack.
*     _NU_USBH_STACK_Is_Endp_Stalled         returns endpoint stall status.
*     _NU_USBH_STACK_Is_Valid_Device         checks validity of Device
*                                            control block.
*     _NU_USBH_STACK_Lock                    grabs a reader/writer lock in
*                                            read mode.
*     _NU_USBH_STACK_Register_Drvr           register a driver to stack.
*     _NU_USBH_STACK_Set_Config              send set config request.
*     _NU_USBH_STACK_Set_Intf                send set interface request.
*     _NU_USBH_STACK_Stall_Endp              send set feature halt request
*     _NU_USBH_STACK_Submit_IRP              submit a IRP onto a pipe.
*     _NU_USBH_STACK_Unlock                  releases reader/writer lock.
*     _NU_USBH_STACK_Unstall_Endp            send clear halt feature request.
*     _NU_USBH_STACK_Flush_Pipe              flushes all irp's from pipe.
*     _NU_USBH_STACK_Get_Configuration       gets configuration from device.
*     _NU_USBH_STACK_Get_Device_Status       sends get status device request.
*     _NU_USBH_STACK_Get_Endpoint_Status     sends get status endpoint
*                                            request.
*     _NU_USBH_STACK_Get_Interface           sends get interface request.
*     _NU_USBH_STACK_Get_Interface_Status    sends get status interface
*                                            request.
*     NU_USBH_STACK_Suspend_Bus              suspends the given port
*                                            (deprecated).
*     NU_USBH_STACK_Suspend_Device           suspends the given device.
*     NU_USBH_STACK_Resume_Device            resumes the given device.
*     _NU_USBH_STACK_Start_Session           starts a session .
*     _NU_USBH_STACK_End_Session             ends a session.
*     NU_USBH_STACK_Get_Devices              returns device list attached
*                                            to the host.
*     NU_USBH_STACK_U1_Enable                This function enables U1
*                                            transition on the device link.
*     NU_USBH_STACK_U2_Enable                This function enables U2
*                                            transition on the device link.
*     NU_USBH_STACK_LTM_Enable               This function enables the LTM
*                                            generation feature of the the
*                                            device.
*     NU_USBH_STACK_U1_Disable               This function disables U1
*                                            transition on the device link.
*     NU_USBH_STACK_U2_Disable               This function disables U2
*                                            transition on the device link.
*     NU_USBH_STACK_LTM_Disable              This function disables the LTM
*                                            generation feature of the the
*                                            device.
*     _NU_USBH_STACK_Function_Suspend        This function sends
*                                            set_feature(Function_Suspend)
*                                            request to the device.
*
* DEPENDENCIES
*
*     nu_usb.h                               All USB definitions
*
************************************************************************/
#ifndef USBH_STACK_EXT_C
#define USBH_STACK_EXT_C

/* ==============  Standard Include Files ============================  */
#include    "connectivity/nu_usb.h"

/* ====================  Function Definitions ========================= */

/*************************************************************************
* FUNCTION
*        NU_USBH_STACK_Create
*
* DESCRIPTION
*       This function creates a Nucleus USB Host Stack. All NU_USBH_STACK
*       services can be used once it is created using this call. This
*       creates a Host Stack task. Initializes Hub class driver and makes a
*       stack ready for adding controllers and registering class drivers.
*
* INPUTS
*       cb                  pointer to host stack control block
*       name                null terminated string to name this instance
*       stack_task_stack_address    pointer to host stack task's stack area
*       stack_task_stack_size       host stack task's stack size
*       stack_task_priority         host stack task's priority
*       hub_task_stack_address      pointer to stack area for hub task
*       hub_task_stack_size         hub task's stack size.
*       hub_task_priority           priority of hub task.
*       hisr_stack_address          stack address for USB Host stack HISR.
*       hisr_stack_size             stack size for USB Host stack HISR.
*       hisr_priority               priority of USB Host Stack HISR.
*
* OUTPUTS
*      NU_SUCCESS           Successful request
*      NU_INVALID_MEMORY    Stack pointer is NU_NULL
*      NU_INVALID_SIZE      Stack size is too small
*      NU_INVALID_PRIORITY  -Invalid task priority
*                           -Invalid HISR priority
*
*************************************************************************/
STATUS NU_USBH_STACK_Create (NU_USBH_STACK * cb,
                             CHAR * name,
                             NU_MEMORY_POOL * pool,
                             VOID *stack_task_stack_address,
                             UNSIGNED stack_task_stack_size,
                             OPTION stack_task_priority,
                             VOID *hub_task_stack_address,
                             UNSIGNED hub_task_stack_size,
                             OPTION hub_task_priority,
                             VOID *hisr_stack_address,
                             UNSIGNED hisr_stack_size,
                             OPTION hisr_priority)
{
    STATUS           status = NU_SUCCESS;
    STATUS           internal_sts = NU_SUCCESS;
    UINT8            rollback = 0;
    NU_USBH_CTRL_IRP *irp;
    NU_USBH_SUBSYS   *stack_subsys;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(name);
    NU_USB_MEMPOOLCHK_RETURN(pool);
    NU_USB_PTRCHK_RETURN(stack_task_stack_address);
    NU_USB_PTRCHK_RETURN(hub_task_stack_address);
    NU_USB_PTRCHK_RETURN(hisr_stack_address);
    /* Check if subsystem is created. */
    NU_USB_PTRCHK_RETURN(nu_usbh);

    /* Initialize */
    memset (cb, 0, sizeof (NU_USBH_STACK));
    stack_subsys = &nu_usbh->stack_subsys;

    /* Create base. */
    status = _NU_USB_STACK_Create ((NU_USB_STACK *) cb,
                                   (NU_USB_SUBSYS *) stack_subsys, name,
                                   &usbh_stack_dispatch);
    if (status != NU_SUCCESS)
    {
        rollback = 1;
    }

    if (!rollback)
    {
        /* Setup data. */
        cb->pool = pool;

        /* This binary semaphore ensures that no more than 1 control transfer is
         * outstanding.
         */
        status = NU_Create_Semaphore (&(cb->ctrl_msgs_semaphore),
                                      "USBH-CTRL", 1, NU_PRIORITY);
        if (status != NU_SUCCESS)
        {
            rollback = 2;
        }
    }

    /* This binary semaphore is used to get notified whenever a control
     * transfer is finished.
     */
    if (!rollback)
    {
        status = NU_Create_Semaphore (&(cb->callback_semaphore), "USBH-CBK",
                                      0, NU_PRIORITY);
        if (status != NU_SUCCESS)
        {
            rollback = 3;
        }
    }

    /* This binary Semaphore is used to activate USBH-Stk task, whenever
     * HISR notices a USB Host Controller interrupt.
     */
    if (!rollback)
    {
        status = NU_Create_Semaphore (&(cb->irq_semaphore), "USBH-IRQ",
                                      0, NU_PRIORITY);
        if (status != NU_SUCCESS)
        {
            rollback = 4;
        }
    }

    /* This Reader/Writer lock is used to ensure that USB device
     * structure doesn't get deleted while USBD APIs are operating
     * on it. They all grab it in Read mode. The de-enumeration
     * function grabs it Write mode.
     */
    if (!rollback)
    {
        status = NU_Create_Semaphore(&(cb->lock), "USBHLOCK",
                                     1, NU_PRIORITY);
        if (status != NU_SUCCESS)
        {
            rollback = 5;
        }
    }

    /* Allocate memory for Endpoint 0 IRP. */
    if (!rollback)
    {
        status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                     sizeof(NU_USBH_CTRL_IRP),
                                     (VOID **)&(cb->ctrl_irp));
        if (status != NU_SUCCESS)
        {
            rollback = 6;
        }
    }

    if (!rollback)
    {
        irp = cb->ctrl_irp;
        memset(irp, 0, sizeof(NU_USBH_CTRL_IRP));

        /* Stack's Endpoint 0 irp */
        status = NU_USBH_CTRL_IRP_Create (irp, 0 /* data */ ,
                                 usb_callback_hndlr /* Callback function */ ,
                                 cb,/* context       */
                                 0, /* bmRequestType */
                                 0, /* bRequest      */
                                 0, /* wValue        */
                                 0, /* wIndex        */
                                 0  /* wLength       */ );
        if (status != NU_SUCCESS)
        {
            rollback = 7;
        }
    }

    if (!rollback)
    {
        /* USBH HISR, it activates USBH-Stk by releasing USBHIRQ semaphore.
         */
        cb->usbh_hisr.stack = cb;
        status = NU_Create_HISR ((NU_HISR *) (&(cb->usbh_hisr)),
                            "USBHISR", usbh_hisr, hisr_priority,
                            hisr_stack_address, hisr_stack_size);
        if (status != NU_SUCCESS)
        {
            rollback = 7;
        }
    }

    /* USBH-Stk task. It gets activated whenever an ISR invokes USB
     * HISR, that in turn releases USBIRQ semaphore. This causes this
     * task to invoke ISR of each of the registered Host controllers.
     */
    if (!rollback)
    {
        status = NU_Create_Task (&(cb->usbh_stack_task), "USBH-Stk",
                                 usbh_scan_controllers, 0, cb,
                                 stack_task_stack_address,
                                 stack_task_stack_size,
                                 stack_task_priority,
                                 0 /* No Time Slicing */ ,
                                 NU_PREEMPT, NU_NO_START);
        if (status != NU_SUCCESS)
        {
            rollback = 8;
        }
    }

    if (!rollback)
    {
        status = NU_Resume_Task((NU_TASK *)&cb->usbh_stack_task);
        if (status != NU_SUCCESS)
        {
            rollback = 9;
        }
    }

    /* Register Hub Class driver and Create a Hub Task. Enumeration and
     * de-enumeration of all USB devices is done only through this hub
     * task, thereby ensuring that no more than one enumeration is
     * active at any given time.
     */
    if (!rollback)
    {
        status = NU_USBH_HUB_Create (&(cb->hub_driver), cb, cb->pool,
                                     hub_task_stack_address,
                                     hub_task_stack_size,
                                     hub_task_priority);
        if (status != NU_SUCCESS)
        {
            rollback = 9;
        }
    }

    if (!rollback)
    {
        status = NU_USB_STACK_Register_Drvr ((NU_USB_STACK *) cb,
                                  (NU_USB_DRVR *) (&(cb->hub_driver)));
        if (status != NU_SUCCESS)
        {
            rollback = 10;
        }
    }

    switch (rollback)
    {
        case 10:
            internal_sts = NU_USB_Delete(&cb->hub_driver);

        case 9:
            internal_sts |= NU_Terminate_Task (&(cb->usbh_stack_task));

            internal_sts |= NU_Delete_Task (&(cb->usbh_stack_task));

        case 8:
            internal_sts |= NU_Delete_HISR ((NU_HISR *) (&(cb->usbh_hisr)));

        case 7:
            internal_sts |= USB_Deallocate_Memory (cb->ctrl_irp);

        case 6:
            internal_sts |= NU_Delete_Semaphore (&(cb->lock));

        case 5:
            internal_sts |= NU_Delete_Semaphore (&(cb->irq_semaphore));

        case 4:
            internal_sts |= NU_Delete_Semaphore (&(cb->callback_semaphore));

        case 3:
            internal_sts |= NU_Delete_Semaphore (&(cb->ctrl_msgs_semaphore));

        case 2:
            internal_sts |= _NU_USB_Delete ((NU_USB *) cb);

        case 1:
            break;

        case 0:
            status = NU_SUCCESS;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ((status == NU_SUCCESS) ? internal_sts : status);
}
/*************************************************************************
* FUNCTION
*        _NU_USBH_STACK_Delete
*
* DESCRIPTION
*     This function deletes all the resources created by the Host stack
*     if there is no class driver or controller is attached.If there is
*     any class driver or controller is attched then it must be removed
*     before calling this function otherwise the function would return
*     error.
*
* INPUTS
*    cb             Pointer to host stack control block.
*
* OUTPUTS
*   NU_USB_INVLD_DELETE     Host stack could not be deleted because
                            either class driver is registered or
                            controller driver is attached.
*   NU_SUCCESS              Successful deletion.
*
*************************************************************************/
STATUS _NU_USBH_STACK_Delete (VOID *cb)
{

    STATUS          status     = NU_SUCCESS;
    NU_USB_STACK  * base_stack = NU_NULL;
    NU_USBH_STACK * host_stack = NU_NULL;

    UINT8 i;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);

    base_stack = (NU_USB_STACK *)  cb;
    host_stack = (NU_USBH_STACK *) cb;

    /* If class driver other then hub dirver is registered with
     * the stack.
     */
    if (base_stack->num_class_drivers > 1)
    {
        NU_USER_MODE();
        return (NU_USB_INVLD_DELETE);
    }

    /* If any HW controller is attached with the stack.*/
    for (i = 0; i < NU_USBH_MAX_HW; i++)
    {
        if (host_stack->bus_resources[i].controller != NU_NULL)
        {
            NU_USER_MODE();
            return (NU_USB_INVLD_DELETE);
        }
     }

    if(base_stack->num_class_drivers != 0)
    {
        /* Delete the hub driver which was created during host stack
         * creation.
         */
        status = _NU_USBH_HUB_Delete ((NU_USB_DRVR *)(&(host_stack->hub_driver)));
    }

    status |= NU_Terminate_Task (&(host_stack->usbh_stack_task));

    status |= NU_Delete_Task (&(host_stack->usbh_stack_task));

    status |= NU_Delete_HISR ((NU_HISR *) (&(host_stack->usbh_hisr)));

    status |= USB_Deallocate_Memory (host_stack->ctrl_irp);

    status |= NU_Delete_Semaphore (&(host_stack->lock));

    status |= NU_Delete_Semaphore (&(host_stack->irq_semaphore));

    status |= NU_Delete_Semaphore (&(host_stack->callback_semaphore));

    status |= NU_Delete_Semaphore (&(host_stack->ctrl_msgs_semaphore));

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*        _NU_USBH_STACK_Register_Drvr
*
* DESCRIPTION
*   Registers Host side USB drivers with the host stack. The newly registered
*   driver may be notified, if any unclaimed devices/interfaces that match the
*   new driver are found by the stack.
*
* INPUTS
*   cb              ptr to the host stack  control block to which the driver is
*                   to be registered.
*   class_driver    ptr to the class driver control block.
*
* OUTPUTS
*   NU_SUCCESS      Indicates successful completion.
*
*************************************************************************/
STATUS _NU_USBH_STACK_Register_Drvr (NU_USB_STACK * cb,
                                     NU_USB_DRVR * class_driver)
{
    STATUS          status,internal_sts;
    NU_USBH_STACK  *stack;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(class_driver);

    stack = (NU_USBH_STACK *) cb;

    /* Ensure an atomic update of the driver database */
    status = _NU_USBH_STACK_Lock ((NU_USB_STACK *) stack);

    /* Base Class behavior */
    if (status == NU_SUCCESS)
    {
        status = _NU_USB_STACK_Register_Drvr ((NU_USB_STACK *) stack,
                                              class_driver);

        if(status == NU_SUCCESS)
        {
            usb_attempt_unclaimed_devs (stack, cb->class_driver_list_head);
        }

        internal_sts = _NU_USBH_STACK_Unlock ((NU_USB_STACK *) stack);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ((status == NU_SUCCESS) ? internal_sts : status);
}

/************************************************************************
*
* FUNCTION
*     _NU_USBH_STACK_Deregister_Drvr
*
* DESCRIPTION
*   It deregisters a USB driver that was earlier registered. The
*   de-registration is successful, only if no
* device/interface is currently attached to the driver.
*
* INPUTS
*       cb              ptr to the host stack control block to which the
*                       driver was registered.
*       class_driver    ptr to the class driver control block.
*
* OUTPUTS
*    STATUS   NU_SUCCESS Indicates that the driver has been successfully
*               deregistered.
*            NU_USB_INVLD_ARG Indicates that one or more args passed to this
*            function are invalid.
*             NU_USB_DRVR_ACTV  Indicates that the driver cannot be
*             deregistered as it still holds one or more interfaces/devices
*             NU_INVALID_SEMAPHORE Indicates the semaphore pointer is
*                   invalid.
*             NU_SEMAPHORE_DELETED Semaphore was deleted while the task
*                   was suspended.
*             NU_UNAVAILABLE Indicates the semaphore is unavailable.
*             NU_INVALID_SUSPEND Indicates that this API is called from
*                a non-task thread.
*
*************************************************************************/
STATUS _NU_USBH_STACK_Deregister_Drvr (NU_USB_STACK * cb,
                                       NU_USB_DRVR * class_driver)
{
    STATUS         status;
    STATUS         internal_sts = NU_SUCCESS;
    NU_USBH_STACK  *stack;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(class_driver);

    stack = (NU_USBH_STACK *) cb;

    status = _NU_USBH_STACK_Lock ((NU_USB_STACK *) stack);
    if (status == NU_SUCCESS)
    {
        /* see if there is device using this class driver */
        if (usb_any_claimed_device (stack, class_driver))
        {
            /* if so, error code is returned */
            status = NU_USB_DRVR_ACTV;
        }
    }
    else
    {
        /* Switch back to user mode. */
        NU_USER_MODE();
        return (status);
    }

    if (status == NU_SUCCESS)
    {
       /* call base stack deregister function to update driver list */
       status = _NU_USB_STACK_Deregister_Drvr((NU_USB_STACK *) stack,
                                               class_driver);
    }

    internal_sts = _NU_USBH_STACK_Unlock ((NU_USB_STACK *) stack);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ((status == NU_SUCCESS) ? internal_sts : status);
}

/*************************************************************************
* FUNCTION
*        _NU_USBH_STACK_Add_Hw
*
* DESCRIPTION
*    Adds a new Host controller to the host stack. The controller is made
*    operational and the root hub contained in the controller is enumerated.
*
* INPUTS
*       cb          ptr to the stack control block.
*       ctrller     ptr to the h/w control block.
*
* OUTPUTS
*
*   STATUS   NU_SUCCESS Indicates that the driver has been successfully
*               deregistered.
*             NU_USB_MAX_EXCEEDED indicates that more than configured
*             number of host controllers are being added.
*            NU_USB_INVLD_ARG Indicates that one or more args passed to this
*            function are invalid.
*             NU_INVALID_SEMAPHORE Indicates the semaphore pointer is
*                   invalid.
*             NU_SEMAPHORE_DELETED Semaphore was deleted while the task
*                   was suspended.
*             NU_UNAVAILABLE Indicates the semaphore is unavailable.
*             NU_INVALID_SUSPEND Indicates that this API is called from
*                a non-task thread.
*
*************************************************************************/
STATUS _NU_USBH_STACK_Add_Hw (NU_USB_STACK * cb,
                              NU_USB_HW * cntrller)
{
    STATUS              status = NU_USB_MAX_EXCEEDED;
    STATUS              internal_sts = NU_SUCCESS;
    NU_USBH_HW          *controller;
    NU_USBH_STACK       *stack;
    UINT8               speed, i, j;
    NU_USB_DEVICE       root_hub_parent;
    USBH_BUS_RESOURCES  *bus = NU_NULL;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(cntrller);

    controller  = (NU_USBH_HW *) cntrller;
    stack       = (NU_USBH_STACK *) cb;
    speed       = USB_SPEED_UNKNOWN;

    status = _NU_USBH_STACK_Lock ((NU_USB_STACK *) stack);

    if (status == NU_SUCCESS)
    {
        /* assume the slots are full */
        status = NU_USB_MAX_EXCEEDED;

        /* Locate a free slot in stack's bus resource map */
        for (i = 0; i < NU_USBH_MAX_HW; i++)
        {
            if (stack->bus_resources[i].controller == NU_NULL)
            {
                /* Capture the slot found in the bus resource map. */
                bus = &stack->bus_resources[i];

                bus->controller  = controller;
                bus->dev_list    = NU_NULL;
                bus->last_dev_id = 1;
                bus->bus_id      = i;

                for (j = 0; j < USB_MAX_DEVID / 8; j++)
                {
                    bus->dev_ids[j] = 0;
                }

                /* address 0 is for the device in 'default' state */
                bus->dev_ids[0] = 1;

                /* root hub has its own address (1 by default) */
                bus->dev_ids[USB_ROOT_HUB / 8] |= 1 << (USB_ROOT_HUB % 8);

                status = NU_SUCCESS;
                break;
            }
        }
    }
    else
    {
        /* Switch back to user mode. */
        NU_USER_MODE();
        return (status);
    }

    if (status == NU_SUCCESS)
    {
        /* Initialize the controller h/w and make it operational */
        status = NU_USB_HW_Initialize ((NU_USB_HW *) controller,
                                       (NU_USB_STACK *) stack);
    }

    if (status == NU_SUCCESS)
    {
        internal_sts = NU_USB_HW_Get_Speed ((NU_USB_HW *) controller, &speed);

        /* B/W available for periodic transfers depends on the speed
         * of the host controller.
         */
        if ((speed == USB_SPEED_FULL) || (speed == USB_SPEED_LOW))
        {
            bus->total_bandwidth = USB1_BANDWIDTH;
        }
        else
        {
            bus->total_bandwidth = USB2_BANDWIDTH;
        }
        bus->avail_bandwidth = bus->total_bandwidth;

        internal_sts |= NU_USB_HW_Enable_Interrupts ((NU_USB_HW *) controller);
    }

    internal_sts |= _NU_USBH_STACK_Unlock ((NU_USB_STACK *) stack);

    /* Create a fake parent device for the root hub */
    if (status == NU_SUCCESS)
    {
        root_hub_parent.hw = (NU_USB_HW *) controller;
        root_hub_parent.stack = (NU_USB_STACK *) stack;
        root_hub_parent.parent = NU_NULL;
        root_hub_parent.function_address = 0;

        /* Since the SuperSpeed hub is a logical combination of two hubs
         * one for handling SuperSpeed devices and other for handling
         * high, low and full speed devices therefore we are required to
         * enumerate two hubs one for handling all device speeds.
         * First enumerate the high speed root hub.
         */
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

        /* Enumerate the SuperSpeed root hub */
        status = USBH_Enumerate_Device (stack, &root_hub_parent, 0,
                                    USB_SPEED_SUPER, &(bus->ss_root_hub));
        speed = USB_SPEED_HIGH;

#endif

        if ( status == NU_SUCCESS )
        {
            status = USBH_Enumerate_Device (stack, &root_hub_parent,
                                            0, speed, &(bus->root_hub));
        }

    }

    if(status != NU_SUCCESS && status != NU_USB_MAX_EXCEEDED)
    {
        stack->bus_resources[i].controller = NU_NULL;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ((status == NU_SUCCESS) ? internal_sts : status);
}

/*************************************************************************
* FUNCTION
*        _NU_USBH_STACK_Remove_Hw
*
* DESCRIPTION
*     Removes a host controller hardware that was earlier added to the stack.
*     The root hub is de-enumerated causing the whole associated device topology
*     to be de-enumerated.
*
* INPUTS
*       cb          ptr to the stack control block.
*       ctrller     ptr to the h/w control block.
*
* OUTPUTS
*       NU_SUCCESS      Indicates successful completion
*       NU_NOT_PRESENT  Indicates HW not added.
*
*************************************************************************/
STATUS _NU_USBH_STACK_Remove_Hw (NU_USB_STACK * cb,
                                 NU_USB_HW * cntrller)
{
    STATUS         internal_sts = NU_SUCCESS;
    STATUS         status = NU_SUCCESS;
    NU_USBH_HW     *controller;
    NU_USBH_STACK  *stack;
    UINT8          i;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(cntrller);

    controller = (NU_USBH_HW *) cntrller;
    stack = (NU_USBH_STACK *) cb;

    status = _NU_USBH_STACK_Lock ((NU_USB_STACK *) stack);
    if (status == NU_SUCCESS)
    {
        /* Locate the HW's entry in the stack's bus resources map */
        for (i = 0; i < NU_USBH_MAX_HW; i++)
        {
            if (stack->bus_resources[i].controller == controller)
                break;
        }
        internal_sts = _NU_USBH_STACK_Unlock ((NU_USB_STACK *) stack);

        if (i == NU_USBH_MAX_HW)
        {
            status = NU_NOT_PRESENT;
        }
        else
        {
            /* Deenumerate the root hub */
            internal_sts |= NU_USBH_HUB_Disconnect (&stack->hub_driver,
                                    stack->bus_resources[i].root_hub);
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

            /* Deenumerate the SuperSpeed root hub */
            internal_sts |= NU_USBH_HUB_Disconnect (&stack->hub_driver,
                                    stack->bus_resources[i].ss_root_hub);
#endif

            /* Stop the controller h/w */
            internal_sts |= NU_USB_HW_Uninitialize ((NU_USB_HW *) controller);

            status = _NU_USBH_STACK_Lock ((NU_USB_STACK *) stack);
            /* Release the slot in the bus resource map */
            if (status == NU_SUCCESS)
            {
                stack->bus_resources[i].controller = NU_NULL;
                /* Unlock the stack. */
                status = _NU_USBH_STACK_Unlock ((NU_USB_STACK *) stack);
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ((status == NU_SUCCESS)? internal_sts : status);
}

/*************************************************************************
*
* FUNCTION
*    _NU_USB_STACK_Set_Config
*
* DESCRIPTION
*    Set the configuration to the specified bConfigurationValue. If the
* specified value is 0, the device is un-configured. This API is meant
* to be used only by device drivers and not by interface drivers.
*
* INPUTS
*   cb              ptr to stack control block.
*   dev             ptr to the device control block.
*   cnfg_number     bConfigurationValue of the config descriptors
*                   that needs to be made active on the device. A
*                   of 0 indicates that the device should be
*                   un-configured.
* OUTPUTS
*     STATUS  NU_SUCCESS Indicates successful completion of the service
*             NU_USB_NO_BANDWIDTH Indicates that the device's b/w
*                requirements of the specified configuration cannot be
*                met by the HC.
*             NU_INVALID_SEMAPHORE Indicates the semaphore pointer is
*                   invalid.
*             NU_SEMAPHORE_DELETED Semaphore was deleted while the task
*                   was suspended.
*             NU_UNAVAILABLE Indicates the semaphore is unavailable.
*             NU_INVALID_SUSPEND Indicates that this API is called from
*                a non-task thread.
*            NU_USB_INVLD_ARG Indicates that one or more args passed to this
*            function are invalid.
*
*************************************************************************/
STATUS _NU_USBH_STACK_Set_Config (NU_USB_STACK * cb,
                                  NU_USB_DEVICE * dev,
                                  UINT8 cnfg_number)
{
    STATUS              status = NU_SUCCESS;
    USBH_BUS_RESOURCES  *bus;
    NU_USB_CFG          *cnfg = NU_NULL;
    UINT8               i, prev;
    NU_USBH_STACK       *stack;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(dev);

    stack = (NU_USBH_STACK *) cb;

    /* find the bus associated with the device */
    bus = usbh_find_bus(stack, dev);
    if(bus == NU_NULL)
    {
        /* Switch back to user mode. */
        NU_USER_MODE();
        return NU_USB_INVLD_ARG;
    }

    /* config_num 0 signifies, un-setting the configuration on device. */
    if (cnfg_number == 0)
    {
        cnfg = NU_NULL;
        i = NU_USB_MAX_CONFIGURATIONS;
    }
    else
    {
        /* Find the configuration that corresponds to the specified
         * cfg_number.
         */
        for (i = 0; i < dev->device_descriptor.bNumConfigurations; i++)
        {
            if ((dev->config_descriptors[i]) &&
                (dev->config_descriptors[i]->desc->bConfigurationValue ==
                     cnfg_number))
            {
                cnfg = dev->config_descriptors[i];
                break;
            }
        }

        if (i == dev->device_descriptor.bNumConfigurations)
        {
            status = NU_USB_INVLD_ARG;
        }
    }

    if ((status == NU_SUCCESS) && (cnfg))
    {
        /* Attempt the setting */
        status = usb_set_config (stack, dev, bus, cnfg);

        /* Mark the active configuration number in the dev structure. */
        if (status == NU_SUCCESS)
        {
            prev = dev->active_cnfg_num;
            dev->active_cnfg_num = i;
            if (prev != NU_USB_MAX_CONFIGURATIONS)
                dev->config_descriptors[prev]->is_active = 0;
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*        _NU_USBH_STACK_Get_Configuration
*
* DESCRIPTION
*       Gets the bConfigurationNumber of the currently active configuration
*  of the device. 0 indicates that it's un-configured.
*
* INPUTS
*   cb          ptr to stack control block.
*   device      ptr to the device control block.
*   cnfgno_out  on successful return it contains , bConfigurationValue of
*               the active config descriptor on the device. A value of 0
*               indicates that the device is un-configured.
* OUTPUTS
*     STATUS  NU_SUCCESS Indicates successful completion of the service
*             NU_INVALID_SEMAPHORE Indicates the semaphore pointer is
*                   invalid.
*             NU_SEMAPHORE_DELETED Semaphore was deleted while the task
*                   was suspended.
*             NU_UNAVAILABLE Indicates the semaphore is unavailable.
*             NU_INVALID_SUSPEND Indicates that this API is called from
*                a non-task thread.
*            NU_USB_INVLD_ARG Indicates that one or more args passed to this API
*            are invalid .
*
*************************************************************************/
STATUS _NU_USBH_STACK_Get_Configuration (NU_USB_STACK * cb,
                                         NU_USB_DEVICE * device,
                                         UINT8 *cnfgno_out)
{
    STATUS         status = NU_SUCCESS;
    STATUS         internal_sts = NU_SUCCESS;
    NU_USBH_STACK  *stack;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(device);
    NU_USB_PTRCHK_RETURN(cnfgno_out);

    stack = (NU_USBH_STACK *) cb;

    status = NU_USB_DEVICE_Lock(device);
    if (status == NU_SUCCESS)
    {
        if (_NU_USBH_STACK_Is_Valid_Device ((NU_USB_STACK *) stack, device))
        {
            /* Base class behavior */
            status = _NU_USB_STACK_Get_Config (cb, device, cnfgno_out);
        }
        else
        {
            status = NU_USB_INVLD_ARG;
        }

        internal_sts = NU_USB_DEVICE_Unlock(device);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ((status == NU_SUCCESS) ? internal_sts : status);
}

/*************************************************************************
*
* FUNCTION
*    _NU_USB_STACK_Set_Intf
*
* DESCRIPTION
*    Set the specified interface to the specified bAlternateSetting.
* This API if used by interface drivers, they should restrict
* to their own interface of the device. A device driver doesn't have
* have this restriction. The interface number and alternate setting
* number refer to the currently active configuration on the device.
*
* INPUTS
*       cb                  ptr to stack control block.
*       dev                 ptr to device control block.
*       interface_index     interface number on the specified device
*                           in the  range of 0 to USB_MAX_INTERFACES-1
*       alt_setting_index   bAlternateSetting value for the interface
*                           in the range of 0 to USB_MAX_ALT_SETTINGS-1
*
* OUTPUTS
*     STATUS  NU_SUCCESS Indicates successful completion of the service
*             NU_USB_NO_BANDWIDTH Indicates that the device's b/w
*                requirements of the specified configuration cannot be
*                met by the HC.
*             NU_INVALID_SEMAPHORE Indicates the semaphore pointer is
*                   invalid.
*             NU_SEMAPHORE_DELETED Semaphore was deleted while the task
*                   was suspended.
*             NU_UNAVAILABLE Indicates the semaphore is unavailable.
*             NU_INVALID_SUSPEND Indicates that this API is called from
*                a non-task thread.
*            NU_USB_INVLD_ARG Indicates that one or more args passed to this
*            function are invalid.
*
*************************************************************************/
STATUS _NU_USBH_STACK_Set_Intf (NU_USB_STACK * cb,
                                NU_USB_DEVICE * dev,
                                UINT8 interface_index,
                                UINT8 alt_setting_index)
{
    USBH_BUS_RESOURCES  *bus = NU_NULL;
    STATUS              status = NU_SUCCESS;
    STATUS              internal_sts = NU_SUCCESS;
    NU_USBH_STACK       *stack = (NU_USBH_STACK *) cb;
    NU_USB_CFG          *cnfg = NU_NULL;

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_FALSE )
    NU_USB_ENDP_DESC    *ep;
    UINT8               direction_n_type;
    UINT16              max_pkt_size;
    NU_USB_ALT_SETTG    *alt_set = NU_NULL,*alt_set_current = NU_NULL;
    UINT32              alt_set_load = 0;
    UINT32              j;
#endif

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(dev);

    if(interface_index >= NU_USB_MAX_INTERFACES ||
       alt_setting_index >= NU_USB_MAX_ALT_SETTINGS)
    {
        /* Switch back to user mode. */
        NU_USER_MODE();
        return NU_USB_INVLD_ARG;
    }

    status = NU_USB_DEVICE_Lock(dev);

    if (status == NU_SUCCESS)
    {
        /* Find the bus to which the device is associated */
        bus = usbh_find_bus (stack, dev);

        /* Get the active configuration pointer */
        cnfg = dev->config_descriptors[dev->active_cnfg_num];
    }
    else
    {
        /* Switch back to user mode. */
        NU_USER_MODE();
        return (status);
    }

    if((cnfg == NU_NULL) || (bus == NU_NULL))
    {
        status = NU_USB_INVLD_ARG;
    }

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

    if ( status ==  NU_SUCCESS)
    {
        /* Since SuperSpeed host controllers manage BW themselves therefore
         * there is no need of calculating BW required for desired interface
         * setting.
         */
        status = usb_set_interface_id (stack, dev, bus,
                             dev->config_descriptors[dev->active_cnfg_num],
                             interface_index, alt_setting_index, NU_TRUE);
    }

#else

    if (status == NU_SUCCESS)
    {
        /* Get pointer for the alt_sttg to be set */
        alt_set = &(cnfg->intf[interface_index].alt_settg[alt_setting_index]);

        /* Get the current alt_settg pointer */
        alt_set_current = cnfg->intf[interface_index].current;

        /* update available bandwidth for the bus */
        if(alt_set_current != NU_NULL)
        {
            bus->avail_bandwidth += alt_set_current->load;
            cnfg->load -= alt_set_current->load;
        }
    }

    if ((status == NU_SUCCESS)&&(alt_set_current != NU_NULL))
    {
        for (j = 0; j < NU_USB_MAX_ENDPOINTS; j++)
        {
            ep = alt_set->endp[j].desc;

            if (ep)  /* Valid endpoint */
            {
                direction_n_type = (UINT8)usb_calc_direction_n_type (
                                       ep->bEndpointAddress, ep->bmAttributes);

                max_pkt_size = ep->wMaxPacketSize0;
                max_pkt_size |= (((UINT16)ep->wMaxPacketSize1) << 8);
                alt_set->endp[j].load = usb_calc_load (direction_n_type,
                                                       dev->speed,
                                             /* Mult times Max Pkt Size */
                               (UINT16)((1 + (((max_pkt_size) >> 11) & 0x03)) *
                               (max_pkt_size & 0x3ff)));
                alt_set_load += alt_set->endp[j].load;
            }
        }

        /* Update the load for the alt_set */
        alt_set->load = alt_set_load;

        if(bus->avail_bandwidth < alt_set_load)
        {
            if(alt_set_current != NU_NULL)
            {
                /* Update the available bandwidth for the current alt-settg */
                bus->avail_bandwidth -= alt_set_current->load;
                status = NU_USB_NO_BANDWIDTH;
            }
        }
        else
        {
            /* Invoke the internal function that actually sends out the set
            * command and configures the HC's H/W to reflect the device's
            * setting.
            */
           status = usb_set_interface_id (stack, dev, bus,
                         dev->config_descriptors[dev->active_cnfg_num],
                         interface_index, alt_setting_index, NU_TRUE);

            if(status == NU_SUCCESS)
            {
                /* Update the available bandwidth */
                bus->avail_bandwidth -= alt_set_load;
                cnfg->load += alt_set_load;
            }
            else
            {
                if(alt_set_current != NU_NULL)
                {
                    /* Update the available bandwidth for the current alt-settg */
                    bus->avail_bandwidth -= alt_set_current->load;
                    cnfg->load += alt_set_current->load;
                }
            }
        }
    }

#endif

    internal_sts = NU_USB_DEVICE_Unlock(dev);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ((status == NU_SUCCESS) ? internal_sts : status);
}

/*************************************************************************
* FUNCTION
*        _NU_USBH_STACK_Get_Interface
*
* DESCRIPTION
*  gets the currently active alternate setting on the specified interface
*  of the device.
*
* INPUTS
*   cb                      ptr to stack control block.
*   dev                     ptr to device control block.
*   interface_index         interface number on the specified device
*                           in the  range of 0 to USB_MAX_INTERFACES-1
*   alt_setting_index_out   on successful return, it would contain
*                           bAlternateSetting value for the interface
*                           in the range of 0 to USB_MAX_ALT_SETTINGS-1
*
* OUTPUTS
*     STATUS  NU_SUCCESS Indicates successful completion of the service
*             NU_INVALID_SEMAPHORE Indicates the semaphore pointer is
*                   invalid.
*             NU_SEMAPHORE_DELETED Semaphore was deleted while the task
*                   was suspended.
*             NU_UNAVAILABLE Indicates the semaphore is unavailable.
*             NU_INVALID_SUSPEND Indicates that this API is called from
*                a non-task thread.
*            NU_USB_INVLD_ARG Indicates that one or more args passed to this
*            function are invalid.
*
*************************************************************************/
STATUS _NU_USBH_STACK_Get_Interface (NU_USB_STACK * cb,
                                     NU_USB_DEVICE * dev,
                                     UINT8 interface_index,
                                     UINT8 *alt_setting_index_out)
{
    NU_USBH_STACK *stack = (NU_USBH_STACK *) cb;
    STATUS         status;
    STATUS         internal_sts = NU_SUCCESS;
    NU_USB_CFG    *config;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(dev);
    NU_USB_PTRCHK_RETURN(alt_setting_index_out);

    status = NU_USB_DEVICE_Lock(dev);

    if(status == NU_SUCCESS)
    {
        if(!_NU_USBH_STACK_Is_Valid_Device((NU_USB_STACK *) stack, dev))
        {
            status = NU_USB_INVLD_ARG;
        }
    }

    /* Check for valid interface_index. */
    if(status == NU_SUCCESS)
    {
        config = dev->config_descriptors[dev->active_cnfg_num];
        if(interface_index > config->desc->bNumInterfaces)
        {
            status = NU_USB_INVLD_ARG;
        }
    }

    if(status == NU_SUCCESS)
    {
        /* Base class behavior */
        status = _NU_USB_STACK_Get_Intf (cb, dev, interface_index,
                                         alt_setting_index_out);
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

    internal_sts = NU_USB_DEVICE_Unlock(dev);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ((status == NU_SUCCESS) ? internal_sts : status);
}

/*************************************************************************
* FUNCTION
*        _NU_USBH_STACK_Get_Interface_Status
*
* DESCRIPTION
*   Gets the status of the specified interface.
*
* INPUTS
*   cb                  ptr to stack control block.
*   dev                 ptr to device control block.
*   interface_index     interface number on the specified device
*                       in the  range of 0 to USB_MAX_INTERFACES-1
*   reply_out           contains the interface status bit map as defined
*                       by the USB standard.
*
* OUTPUTS
*     STATUS  NU_SUCCESS Indicates successful completion of the service
*             NU_INVALID_SEMAPHORE Indicates the semaphore pointer is
*                   invalid.
*             NU_SEMAPHORE_DELETED Semaphore was deleted while the task
*                   was suspended.
*             NU_UNAVAILABLE Indicates the semaphore is unavailable.
*             NU_INVALID_SUSPEND Indicates that this API is called from
*                a non-task thread.
*            NU_USB_INVLD_ARG Indicates that one or more args passed to this
*            function are invalid.
*
*************************************************************************/
STATUS _NU_USBH_STACK_Get_Interface_Status (NU_USB_STACK * cb,
                                            NU_USB_DEVICE * dev,
                                            UINT8 interface_index,
                                            UINT16 *reply_out)
{
    NU_USBH_STACK *stack        = (NU_USBH_STACK *) cb;
    STATUS         status       = NU_SUCCESS;
    STATUS         internal_sts = NU_SUCCESS;
    BOOLEAN        unlock       = NU_FALSE;

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
    UINT16        *temp_buffer  = 0;
    UINT8          length       = 0;
#endif
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(dev);
    NU_USB_PTRCHK_RETURN(reply_out);

    status = NU_USB_DEVICE_Lock(dev);

    if(status == NU_SUCCESS)
    {
        if(!_NU_USBH_STACK_Is_Valid_Device((NU_USB_STACK *) stack, dev))
        {
            status = NU_USB_INVLD_ARG;
        }

        unlock = NU_TRUE;
    }

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

    if (status == NU_SUCCESS)
    {
        /* Allocate uncached memory buffer for data IO. */
        status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                     sizeof(UINT16),
                                     (VOID **) &temp_buffer);

        if(status == NU_SUCCESS)
        {
            /* Clear out temp buffer. */
            memset(temp_buffer, 0, sizeof(UINT16));

            /* Create a control packet */
            internal_sts = USBH_FILL_CNTRLPKT ((stack->ctrl_irp),
                                (USB_REQ_RECP_INTF | USB_REQ_STD | USB_REQ_IN),
                                USB_GET_STATUS, 0, interface_index, sizeof(UINT16));

            /* Send the Get Status request to the interface. */
            length = usb_control_transfer (stack, &dev->ctrl_pipe,
                                    stack->ctrl_irp, temp_buffer, sizeof(UINT16));
            if (length != (sizeof(UINT16)))
            {
                status = NU_USB_TRANSFER_FAILED;
            }
            else
            {
                status = NU_SUCCESS;
                *reply_out = LE16_2_HOST (*temp_buffer);
            }

            /* Now de-allocate the temporary buffer. */
            internal_sts = USB_Deallocate_Memory(temp_buffer);
        }
    }

#else

    if(status == NU_SUCCESS)
    {
        /* Base class behavior */
        status = _NU_USB_STACK_Get_Intf_Status (cb, dev, interface_index,
                                                reply_out);
    }

#endif      /* ( USB_SPECS_COMPATIBILITY == USB_VERSION_3_0 ) .*/

    if (unlock == NU_TRUE)
    {
        internal_sts = NU_USB_DEVICE_Unlock(dev);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ((status == NU_SUCCESS) ? internal_sts : status);
}

/*************************************************************************
* FUNCTION
*    _NU_USBH_STACK_Stall_Endp
*
* DESCRIPTION
*    Sends a Stall command to the specified endpoint.
*
* INPUTS
*    cb      ptr to the stack control block.
*    pipe    ptr to the pipe control block that identifies the endpoint.
*
* OUTPUTS
*    STATUS  NU_SUCCESS Indicates successful completion of the service
*            NU_INVALID_SEMAPHORE Indicates the semaphore pointer is
*                   invalid.
*            NU_SEMAPHORE_DELETED Semaphore was deleted while the task
*                   was suspended.
*            NU_UNAVAILABLE Indicates the semaphore is unavailable.
*            NU_INVALID_SUSPEND Indicates that this API is called from
*                a non-task thread.
*            NU_USB_INVLD_ARG Indicates that one or more args passed to this
*            function are invalid.
*
*************************************************************************/
STATUS _NU_USBH_STACK_Stall_Endp (NU_USB_STACK * cb,
                                  NU_USB_PIPE * pipe)
{
    NU_USBH_STACK  *stack;
    NU_USB_DEVICE  *dev;
    STATUS         status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(pipe);

    dev = pipe->device;
    stack = (NU_USBH_STACK *) cb;

    /* Make sure that the device is a valid one ! */
    if (_NU_USBH_STACK_Is_Valid_Device ((NU_USB_STACK *) stack, dev))
    {
        status = usb_modify_ep (stack, dev, pipe, USB_SET_FEATURE);
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*        _NU_USBH_STACK_Unstall_Endp
*
* DESCRIPTION
*    Sends a clear stall command to the specified endpoint.
*
* INPUTS
*   cb      ptr to the stack control block.
*   pipe    ptr to the pipe control block that identifies the endpoint.
*
* OUTPUTS
*     STATUS  NU_SUCCESS Indicates successful completion of the service
*             NU_INVALID_SEMAPHORE Indicates the semaphore pointer is
*                   invalid.
*             NU_SEMAPHORE_DELETED Semaphore was deleted while the task
*                   was suspended.
*             NU_UNAVAILABLE Indicates the semaphore is unavailable.
*             NU_INVALID_SUSPEND Indicates that this API is called from
*                a non-task thread.
*            NU_USB_INVLD_ARG Indicates that one or more args passed to this
*            function are invalid.
*
*************************************************************************/
STATUS _NU_USBH_STACK_Unstall_Endp (NU_USB_STACK * cb,
                                    NU_USB_PIPE * pipe)
{
    NU_USBH_STACK  *stack;
    NU_USB_DEVICE  *dev;
    STATUS         status = NU_SUCCESS;
    UINT8          bEndpointAddress;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(pipe);

    stack = (NU_USBH_STACK *) cb;
    dev = pipe->device;

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
    if (_NU_USBH_STACK_Is_Valid_Device ((NU_USB_STACK *) stack, dev))
    {
        /* Flush pipe if it is BULK or interrupt endpoints */
        if (pipe->endpoint != NU_NULL)
        {
            bEndpointAddress = pipe->endpoint->desc->bEndpointAddress;

            /* invoke H/W's function */
            status = NU_USBH_HW_Unstall_Pipe ((NU_USBH_HW *)dev->hw, bEndpointAddress,
                                               dev->function_address );
        }
        if ( status == NU_SUCCESS )
        {
            status = usb_modify_ep (stack, dev, pipe, USB_CLEAR_FEATURE);
        }
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

#else

    if (_NU_USBH_STACK_Is_Valid_Device ((NU_USB_STACK *) stack, dev))
    {
        status = usb_modify_ep (stack, dev, pipe, USB_CLEAR_FEATURE);
    }
    else
    {
        status = NU_USB_INVLD_ARG;
    }

    if (status == NU_SUCCESS)
    {
        /* Flush pipe if it is BULK or interrupt endpoints */
        if (pipe->endpoint != NU_NULL)
        {
            bEndpointAddress = pipe->endpoint->desc->bEndpointAddress;

            /* invoke H/W's function */
            status = NU_USB_HW_Flush_Pipe (dev->hw, dev->function_address,
                                           bEndpointAddress);
        }
    }
#endif

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*    _NU_USBH_STACK_Get_Endpoint_Status
*
* DESCRIPTION
*    Gets the status of the specified endpoint.
*
* INPUTS
*    cb      ptr to the stack control block.
*    pipe    ptr to the pipe control block that identifies the endpoint.
*
* OUTPUTS
*    STATUS  NU_SUCCESS Indicates successful completion of the service
*            NU_INVALID_SEMAPHORE Indicates the semaphore pointer is
*                   invalid.
*            NU_SEMAPHORE_DELETED Semaphore was deleted while the task
*                   was suspended.
*            NU_UNAVAILABLE Indicates the semaphore is unavailable.
*            NU_INVALID_SUSPEND Indicates that this API is called from
*                a non-task thread.
*            NU_USB_INVLD_ARG Indicates that one or more args passed to this
*            function are invalid.
*
*************************************************************************/
STATUS _NU_USBH_STACK_Get_Endpoint_Status (NU_USB_STACK * cb,
                                           NU_USB_PIPE * pipe,
                                           UINT16 *ep_status)
{
    NU_USBH_STACK *stack;
    STATUS         status = NU_SUCCESS;
    STATUS         internal_sts = NU_SUCCESS;
    NU_USB_DEVICE *dev;
    UINT16        *temp_buffer = NU_NULL;
    UINT32         len;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(pipe);
    NU_USB_PTRCHK_RETURN(ep_status);

    stack = (NU_USBH_STACK *) cb;
    dev = pipe->device;

    if (!_NU_USBH_STACK_Is_Valid_Device ((NU_USB_STACK *) stack, dev))
    {
        status = NU_USB_INVLD_ARG;
    }

    if ( status == NU_SUCCESS)
    {
        if ((pipe->endpoint == NU_NULL) ||
            (dev->active_cnfg_num == NU_USB_MAX_CONFIGURATIONS))
        {
            /* Device not yet configured */
            status = NU_USB_INVLD_ARG;
        }
    }

    if ( status == NU_SUCCESS)
    {
        /* Allocate uncached memory buffer for data IO. */
        status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                     sizeof(UINT16),
                                      (VOID **) &temp_buffer);

        if(status == NU_SUCCESS)
        {
            /* Clear temp buffer. */
            memset(temp_buffer, 0 , sizeof(UINT16));

            internal_sts = USBH_FILL_CNTRLPKT ((stack->ctrl_irp),
                                (USB_REQ_RECP_EP | USB_REQ_STD | USB_REQ_IN),
                                USB_GET_STATUS, 0,
                                (pipe->endpoint->desc->bEndpointAddress ), sizeof(UINT16));

            len = usb_control_transfer (stack, &dev->ctrl_pipe,
                                        stack->ctrl_irp, temp_buffer,
                                        sizeof(UINT16));
            if (len != (sizeof(UINT16)))
            {
                status = NU_USB_TRANSFER_FAILED;
            }
            else
            {
                status = NU_SUCCESS;
                *ep_status = LE16_2_HOST (*temp_buffer);
            }

            /* Now de-allocate the temporary buffer. */
            internal_sts = USB_Deallocate_Memory(temp_buffer);
        }

    }

    NU_UNUSED_PARAM(internal_sts);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*        _NU_USBH_STACK_Is_Endp_Stalled
*
* DESCRIPTION
*  Gets the endpoint status to find out if the endpoint is stalled.
*
* INPUTS
*   cb      ptr to the stack control block.
*   pipe    ptr to the pipe control block that identifies the endpoint.
*
* OUTPUTS
*     STATUS  NU_SUCCESS Indicates successful completion of the service
*             NU_INVALID_SEMAPHORE Indicates the semaphore pointer is
*                   invalid.
*             NU_SEMAPHORE_DELETED Semaphore was deleted while the task
*                   was suspended.
*             NU_UNAVAILABLE Indicates the semaphore is unavailable.
*             NU_INVALID_SUSPEND Indicates that this API is called from
*                a non-task thread.
*            NU_USB_INVLD_ARG Indicates that one or more args passed to this
*            function are invalid.
*
*************************************************************************/
STATUS _NU_USBH_STACK_Is_Endp_Stalled (NU_USB_STACK * stack,
                                       NU_USB_PIPE * pipe,
                                       BOOLEAN * reply)
{
    STATUS  status;
    UINT16  ep_status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(stack);
    NU_USB_PTRCHK_RETURN(pipe);
    NU_USB_PTRCHK_RETURN(reply);

    status = _NU_USBH_STACK_Get_Endpoint_Status (stack, pipe, &ep_status);

    if (status == NU_SUCCESS)
    {
        *reply = (BOOLEAN)(ep_status & 0x0001);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*        _NU_USBH_STACK_Get_Device_Status
*
* DESCRIPTION
*       Gets the status of the specified device.
*
* INPUTS
*   cb          ptr to the stack control block.
*   device      ptr to the device control block.
*   status      on successful return, it would contain the status bit map
*               defined by the USB standard.
*
* OUTPUTS
*     STATUS  NU_SUCCESS Indicates successful completion of the service
*             NU_INVALID_SEMAPHORE Indicates the semaphore pointer is
*                   invalid.
*             NU_SEMAPHORE_DELETED Semaphore was deleted while the task
*                   was suspended.
*             NU_UNAVAILABLE Indicates the semaphore is unavailable.
*             NU_INVALID_SUSPEND Indicates that this API is called from
*                a non-task thread.
*            NU_USB_INVLD_ARG Indicates that one or more args passed to this
*            function are invalid.
*            NU_USB_DEVICE_NOT_RESPONDING Indicates that the device did
*                not respond.
*
*************************************************************************/
STATUS _NU_USBH_STACK_Get_Device_Status (NU_USB_STACK * cb,
                                         NU_USB_DEVICE * device,
                                         UINT16 *device_status)
{
    UINT16         *temp_buffer = NU_NULL;
    STATUS         status;
    STATUS         internal_sts = NU_SUCCESS;
    UINT32         len;
    NU_USBH_STACK  *stack;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(device);
    NU_USB_PTRCHK_RETURN(device_status);

    stack = (NU_USBH_STACK *) cb;

    /* Is the device a valid one? */
    if (!_NU_USBH_STACK_Is_Valid_Device ((NU_USB_STACK *) stack, device))
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        /* Allocate uncached memory buffer for data IO. */
        status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                     sizeof(UINT16),
                                      (VOID **) &temp_buffer);

        if(status == NU_SUCCESS)
        {
            /* Clear temp buffer. */
            memset(temp_buffer, 0, sizeof(UINT16));

            /* Create a control packet */
            internal_sts = USBH_FILL_CNTRLPKT ((stack->ctrl_irp),
                                (USB_REQ_RECP_DEV | USB_REQ_STD | USB_REQ_IN),
                                USB_GET_STATUS, 0, 0, sizeof(UINT16));

            /* Send the Get Status request to the device */
            len = usb_control_transfer (stack, &device->ctrl_pipe,
                                    stack->ctrl_irp, temp_buffer, sizeof(UINT16));
            if (len != (sizeof(UINT16)))
            {
                status = NU_USB_DEVICE_NOT_RESPONDING;
            }
            else
            {
                status = NU_SUCCESS;
                *device_status = LE16_2_HOST (*temp_buffer);
            }

            /* Now de-allocate the temporary buffer. */
            internal_sts = USB_Deallocate_Memory(temp_buffer);
        }
    }

    NU_UNUSED_PARAM(internal_sts);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/************************************************************************
*
* FUNCTION
*     _NU_USBH_STACK_Submit_IRP
*
* DESCRIPTION
*    Invokes H/W's submit IRP API after making sure that the arguments are
*  all valid.
*
* INPUTS
*   cb          ptr to the stack control block.
*   irp         ptr to IRP control block.
*   pipe        ptr to the pipe control block, over which the IRP transfer
*               needs to be made.
*
* OUTPUTS
*     STATUS  NU_SUCCESS Indicates successful completion of the service
*             NU_INVALID_SEMAPHORE Indicates the semaphore pointer is
*                   invalid.
*             NU_SEMAPHORE_DELETED Semaphore was deleted while the task
*                   was suspended.
*             NU_UNAVAILABLE Indicates the semaphore is unavailable.
*             NU_INVALID_SUSPEND Indicates that this API is called from
*                a non-task thread.
*            NU_USB_INVLD_ARG Indicates that one or more args passed to this
*            function are invalid.
*           NU_NOT_PRESENT   Indicates that the pipe is invalid
*           NU_NO_MEMORY Indicates failure of memory allocation
*
*************************************************************************/
STATUS _NU_USBH_STACK_Submit_IRP (NU_USB_STACK * cb,
                                  NU_USB_IRP * irp,
                                  NU_USB_PIPE * pipe)
{
    STATUS              status = NU_SUCCESS;
    USBH_BUS_RESOURCES  *bus;
    NU_USB_DEVICE       *dev;
    NU_USB_ENDP_DESC    *ep;
    NU_USBH_STACK       *stack;
    UINT8               bEndpointAddress;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(irp);
    NU_USB_PTRCHK_RETURN(pipe);

    stack = (NU_USBH_STACK *) cb;
    dev = pipe->device;
    irp->pipe = pipe;

    /* find the bus associated with the device */
    bus = usbh_find_bus (stack, dev);

    /* for non root hubs and non zero endpoints, check if the endpoint specified
     * by pipe has a valid endpoint descriptor in its device database.
     */
    if ((dev->function_address != USB_DEFAULT_DEV_ID) &&
        (dev->function_address != USB_ROOT_HUB) && (pipe->endpoint != NU_NULL))
    {
        ep = usb_verify_pipe (dev, pipe);
        if (ep == NU_NULL)
        {
            status = NU_NOT_PRESENT;
        }
    }

    if ((status == NU_SUCCESS) && (bus))
    {
        /* Endpoint NU_NULL denotes endpoint 0 */
        if (pipe->endpoint == NU_NULL)
        {
            bEndpointAddress = 0x80;
        }
        else
        {
            bEndpointAddress = pipe->endpoint->desc->bEndpointAddress;
        }

        /* invoke H/W's function */
        status = NU_USB_HW_Submit_IRP ((NU_USB_HW *) bus->controller,
                                irp, dev->function_address, bEndpointAddress);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/************************************************************************
*
* FUNCTION
*     _NU_USBH_STACK_Cancel_IRP
*
* DESCRIPTION
*    Invokes H/W's cancel IRP API after making sure that the arguments are
*  all valid.
*
* INPUTS
*   cb          ptr to the stack control block.
*   pipe        ptr to the pipe control block, over which the IRP transfer
*               cancellation is required.
*
* OUTPUTS
*     STATUS  NU_SUCCESS Indicates successful completion of the service
*             Actual flushing of each IRP is notified thru irp's callback.
*             NU_INVALID_SEMAPHORE Indicates the semaphore pointer is
*                   invalid.
*             NU_SEMAPHORE_DELETED Semaphore was deleted while the task
*                   was suspended.
*             NU_UNAVAILABLE Indicates the semaphore is unavailable.
*             NU_INVALID_SUSPEND Indicates that this API is called from
*                a non-task thread.
*            NU_USB_INVLD_ARG Indicates that one or more args passed to this
*            function are invalid.
*
*************************************************************************/
STATUS _NU_USBH_STACK_Cancel_IRP (NU_USB_STACK * cb,
                                  NU_USB_PIPE * pipe)
{
    STATUS              status = NU_SUCCESS;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(pipe);

    if(status == NU_SUCCESS)
    {
        status = _NU_USBH_STACK_Flush_Pipe (cb,pipe);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/************************************************************************
*
* FUNCTION
*     _NU_USBH_STACK_Flush_Pipe
*
* DESCRIPTION
*     This API invokes H/W's API that flushes out all pending IRPs
* associated with the specified pipe. The callback function specified in
* the IRPs would get invoked and irp's status would indicate
* NU_USB_IRP_CANCELLED. Until the callback gets invoked none of IRPs
* should be modified by the caller.
*
* INPUTS
*   cb          ptr to the stack control block.
*   pipe        ptr to the pipe control block, being flushed.
*
* OUTPUTS
*     STATUS  NU_SUCCESS Indicates successful completion of the service
*             Actual flushing of each IRP is notified thru irp's callback.
*             NU_INVALID_SEMAPHORE Indicates the semaphore pointer is
*                   invalid.
*             NU_SEMAPHORE_DELETED Semaphore was deleted while the task
*                   was suspended.
*             NU_UNAVAILABLE Indicates the semaphore is unavailable.
*             NU_INVALID_SUSPEND Indicates that this API is called from
*                a non-task thread.
*            NU_USB_INVLD_ARG Indicates that one or more args passed to this
*            function are invalid.
*
************************************************************************/
STATUS _NU_USBH_STACK_Flush_Pipe (NU_USB_STACK * cb,
                                  NU_USB_PIPE * pipe)
{
    STATUS              status = NU_SUCCESS;
    USBH_BUS_RESOURCES  *bus;
    NU_USB_DEVICE       *dev;
    NU_USB_ENDP_DESC    *ep;
    NU_USBH_STACK       *stack;
    UINT8               bEndpointAddress;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(pipe);

    dev = pipe->device;
    stack = (NU_USBH_STACK *) cb;

    /* find the bus associated with the device. */
    bus = usbh_find_bus (stack, dev);

    /* for non root hubs and non zero endpoints, check if the endpoint specified
     * in the irp->pipe has a valid endpoint descriptor in its device database.
     */
    if ((dev->function_address != USB_DEFAULT_DEV_ID) &&
        (dev->function_address != USB_ROOT_HUB) && (pipe->endpoint != 0))
    {
        ep = usb_verify_pipe (dev, pipe);
        if (ep == NU_NULL)
        {
            status = NU_NOT_PRESENT;
        }
    }

    if ((status == NU_SUCCESS) && (bus))
    {
        /* Endpoint zero . */
        if (pipe->endpoint == NU_NULL)
        {
            bEndpointAddress = 0x80;
        }
        else
        {
            bEndpointAddress = pipe->endpoint->desc->bEndpointAddress;
        }

        /* invoke H/W's function */
        status = NU_USB_HW_Flush_Pipe ((NU_USB_HW *) bus->controller,
                                  dev->function_address, bEndpointAddress);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*        _NU_USBH_STACK_Lock
*
* DESCRIPTION
*     Grabs stack's reader/writer lock in read mode.
*
* INPUTS
*    cb         ptr to stack control block.
*
* OUTPUTS
*       STATUS NU_SUCCESS Indicates successful completion of the service.
*             NU_INVALID_SEMAPHORE Indicates the semaphore pointer is
*                   invalid.
*             NU_SEMAPHORE_DELETED Semaphore was deleted while the task
*                   was suspended.
*             NU_UNAVAILABLE Indicates the semaphore is unavailable.
*             NU_INVALID_POOL Indicates the dynamic memory pool is
*                   invalid.
*             NU_INVALID_SUSPEND Indicates that this API is called from
*                a non-task thread.
*
*************************************************************************/
STATUS _NU_USBH_STACK_Lock (NU_USB_STACK * cb)
{
    NU_USB_PTRCHK(cb);
    return NU_Obtain_Semaphore(&(((NU_USBH_STACK *)cb)->lock), NU_SUSPEND);
}

/*************************************************************************
* FUNCTION
*        _NU_USBH_STACK_Unlock
*
* DESCRIPTION
*     Releases stack's reader/writer lock in read mode.
*
* INPUTS
*    cb     ptr to stack control block.
*
* OUTPUTS
*       STATUS NU_SUCCESS Indicates successful completion of the service.
*             NU_INVALID_SEMAPHORE Indicates the semaphore pointer is
*                   invalid.
*             NU_SEMAPHORE_DELETED Semaphore was deleted while the task
*                   was suspended.
*             NU_UNAVAILABLE Indicates the semaphore is unavailable.
*
*************************************************************************/
STATUS _NU_USBH_STACK_Unlock (NU_USB_STACK * cb)
{
    NU_USB_PTRCHK(cb);
    return NU_Release_Semaphore(&(((NU_USBH_STACK *)cb)->lock));
}

/*************************************************************************
* FUNCTION
*        _NU_USBH_STACK_Is_Valid_Device
*
* DESCRIPTION
*  Checks if the device control block is a valid a one, by seeing if the
*  function address in the control block is valid on its associated bus.
*
* INPUTS
*   cb          ptr to stack control block.
*   device      ptr to device control block.
*
* OUTPUTS
*   return 1 if its a valid device and 0 if not.
*
*************************************************************************/
BOOLEAN _NU_USBH_STACK_Is_Valid_Device (NU_USB_STACK * cb,
                                        NU_USB_DEVICE * device)
{
    NU_USBH_STACK *stack = NU_NULL;

    if((cb == NU_NULL)||(device == NU_NULL))
    {
        return (NU_FALSE);
    }
    stack =  (NU_USBH_STACK *) cb;

    if(usbh_find_bus (stack, device) == NU_NULL)
    {
        return NU_FALSE;
    }
    else
    {
        return NU_TRUE;
    }
}

/*************************************************************************
* FUNCTION
*        NU_USBH_STACK_Suspend_Bus
*
* DESCRIPTION
*       This function suspends the USB traffic (stops sending SOFs) on the
*       bus, on the given port of the hardware.
*       Callers must make sure that there are no active transfers on the
*       USB. Otherwise this may lead to data loss.
*       A value of 0xff (255) for the port_id parameter results in suspend
*       of all the available ports of the hardware.
*
* INPUTS
*       cb          ptr to stack control block.
*       hw          ptr to HW control block.
*       port_id     Port on which suspend is to be implemented.
*
* OUTPUTS
*       return 1 if its a valid device and 0 if not.
*
*************************************************************************/
STATUS NU_USBH_STACK_Suspend_Bus (NU_USBH_STACK *cb,
                                  NU_USBH_HW *hw,
                                  UINT8 port_id)
{
    STATUS         status = NU_USB_INVLD_ARG;
    STATUS         internal_sts = NU_SUCCESS;
    UINT32         len = 0;
    INT            i;
    UINT8          role = 0;
    NU_USB_DEVICE  *root_hub = NU_NULL;
    NU_USB_DEVICE  *dev;
    NU_USB_PIPE    *pipe;
    UINT8          t_port = port_id;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(hw);

    dev = NU_NULL;

    if(port_id == 0xff)
    {
        t_port = 0x01;
    }

    for(i = 0; i < NU_USBH_MAX_HW; i++)
    {
        if(cb->bus_resources[i].controller == hw)
        {
            root_hub =  cb->bus_resources[i].root_hub;

            /* find the device connected on this port */
            dev = cb->bus_resources[i].dev_list;

            while(dev)
            {
                if((dev->parent == root_hub) && (dev->port_number == t_port))
                {
                    break;
                }

                dev = (NU_USB_DEVICE *) dev->node.cs_next;
                if (dev == cb->bus_resources[i].dev_list)
                {
                    dev = NU_NULL;
                }
           }
           break;
        }
    }

    if((root_hub != NU_NULL) && (dev != NU_NULL))
    {
        /* If the device is OTG capable, send set_feature(b_hnp_support) */
        status = NU_USB_DEVICE_Lock(dev);
        if(status == NU_SUCCESS)
        {
            status = NU_USB_HW_Get_Role((NU_USB_HW *)hw, port_id, &role);
        }

        if(status == NU_SUCCESS)
        {
            if((role & 0x02) && (dev->otg_status & (0x40)))
            {
                pipe = &dev->ctrl_pipe;
                internal_sts = USBH_FILL_CNTRLPKT ((cb->ctrl_irp), 0x0, USB_SET_FEATURE,
                                     0x03, 0, 0);
                len = usb_control_transfer (cb, pipe, cb->ctrl_irp, NU_NULL, 0);

                internal_sts |= NU_USB_IRP_Get_Status((NU_USB_IRP *)&cb->ctrl_irp, &status);
                if(status == NU_SUCCESS)
                {
                   dev->otg_status |= (0x03 << 4);
                }
                /* Check if set_feature(B_HNP_ENABLE) is stalled. */
                else if(status == NU_USB_STALL_ERR)
                {
                    /* Report status that B_HNP_ENABLE feature is stalled
                       by B-Function. */
                    internal_sts |= NU_USB_STACK_OTG_Report_Status((NU_USB_STACK *) cb,
                                                NU_USB_HNP_ENABLE_REJECTED);
                }
            }

            status = usbh_hub_suspend_port(&cb->hub_driver, root_hub, port_id);
            if(status == NU_SUCCESS)
            {
                dev->state |= USB_STATE_SUSPENDED;
            }
        }
        NU_UNUSED_PARAM(internal_sts);
        NU_UNUSED_PARAM(len);

        internal_sts = NU_USB_DEVICE_Unlock(dev);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ((status == NU_SUCCESS) ? internal_sts : status);
}

/*************************************************************************
* FUNCTION
*        NU_USBH_STACK_Suspend_Device
*
* DESCRIPTION
*       This function suspends the USB traffic (stops sending SOFs) on the
*       bus, on the given device.
*       Callers must make sure that there are no active transfers on the
*       USB. Otherwise this may lead to data loss.
*
* INPUTS
*       cb          ptr to stack control block.
*       dev         ptr to device control block
*
* OUTPUTS
*       return 1 if its a valid device and 0 if not.
*
*************************************************************************/
STATUS NU_USBH_STACK_Suspend_Device (NU_USBH_STACK *cb,
                                     NU_USB_DEVICE *dev)
{
    STATUS         status = NU_USB_INVLD_ARG;
    STATUS         internal_sts = NU_SUCCESS;
    UINT32         len = 0;
    UINT8          role;
    NU_USB_DEVICE  *root_hub = NU_NULL;
    NU_USB_PIPE    *pipe;
    NU_USBH_HW     *hw;
    UINT8          t_port;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(dev);
    NU_USB_PTRCHK_RETURN(dev->parent);

    t_port = dev->port_number;

    /* Traverse down the hubs until we arrive at the root hub */
    root_hub = dev->parent;
    while(root_hub->parent != NU_NULL)
    {
        root_hub = root_hub->parent;
    }

    if(t_port == 0xff)
        t_port = 0x01;

    status = NU_USB_DEVICE_Lock(dev);
    if (status == NU_SUCCESS)
    {
        hw = (NU_USBH_HW *) (dev->hw);

        if(dev->parent == root_hub)
        {
            /* if the device is an OTG capable device and can be enabled,
             * we send a set_feature(b_hnp_enable) now.
             */
            status = NU_USB_HW_Get_Role((NU_USB_HW *)hw, t_port, &role);
            if(status == NU_SUCCESS)
            {
                if((role & 0x02) && (dev->otg_status & (0x40)))
                {
                    pipe = &dev->ctrl_pipe;
                    internal_sts = USBH_FILL_CNTRLPKT ((cb->ctrl_irp), 0x0, USB_SET_FEATURE,
                                         0x03, 0, 0);
                    len = usb_control_transfer (cb, pipe, cb->ctrl_irp, NU_NULL, 0);

                    internal_sts |= NU_USB_IRP_Get_Status((NU_USB_IRP *)&cb->ctrl_irp, &status);
                    if(status == NU_SUCCESS)
                    {
                        dev->otg_status |= (0x03 << 4);
                    }

                    /* Check if set_feature(B_HNP_ENABLE) is stalled. */
                    else if(status == NU_USB_STALL_ERR)
                    {
                       /* Report status that B_HNP_ENABLE feature is stalled
                          by B-Function. */
                        internal_sts |= NU_USB_STACK_OTG_Report_Status((NU_USB_STACK *) cb,
                                                NU_USB_HNP_ENABLE_REJECTED);
                    }
                }
            }
        }
        status = usbh_hub_suspend_port(&cb->hub_driver, dev->parent, t_port);

        if(status == NU_SUCCESS)
            dev->state |= USB_STATE_SUSPENDED;

        NU_UNUSED_PARAM(internal_sts);
        NU_UNUSED_PARAM(len);

        internal_sts = NU_USB_DEVICE_Unlock(dev);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ((status == NU_SUCCESS) ? internal_sts : status);
}

/*************************************************************************
* FUNCTION
*
*        NU_USBH_STACK_Resume_Device
*
* DESCRIPTION
*
*       This function resumes the USB traffic (restarts sending SOFs) on the
*       bus, on the given port of the hardware.
*
* INPUTS
*
*       cb          ptr to stack control block.
*       hw          ptr to HW control block.
*       port_id     Port on which resume is to be implemented.
*
* OUTPUTS
*
*       return 1 if its a valid device and 0 if not.
*
*************************************************************************/
STATUS NU_USBH_STACK_Resume_Device (NU_USBH_STACK *cb,
                                    NU_USB_DEVICE *dev)
{
    STATUS  status = NU_USB_INVLD_ARG;
    STATUS  internal_sts = NU_SUCCESS;
    UINT8   t_port;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(dev);

    if(dev->parent == NU_NULL)
    {
        return (status);
    }

    t_port = dev->port_number;

    if(t_port == 0xff)
        t_port = 0x01;

    status = NU_USB_DEVICE_Lock(dev);

    if (status == NU_SUCCESS)
    {
        status = usbh_hub_resume_port(&cb->hub_driver, dev->parent, t_port);
    }

    dev->state &= ~(USB_STATE_SUSPENDED);
    internal_sts = NU_USB_DEVICE_Unlock(dev);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ((status == NU_SUCCESS) ? internal_sts : status);
}

/*************************************************************************
* FUNCTION
*       _NU_USBH_STACK_Start_Session
*
* DESCRIPTION
*        This function is invoked by Applications when they wish to request a
*        session. It checks if the SRP is enabled and invokes hardware
*        drivers Start SRP service.
*
* INPUTS
*       cb                      Stack control block.
*       fc                      Hardware on which SRP is to be initiated
*       port_id                 port identifier on which SRP is to be initiated
*       delay                   Time in ms for which starting a session will be
*                               attempted.
*
* OUTPUTS
*       NU_SUCCESS              The service could be executed successfully.
*       NU_USB_INVLD_ARG        Specified port could not be found
*       NU_USB_NOT_SUPPORTED    This service is not supported by the FC
*
*************************************************************************/
STATUS _NU_USBH_STACK_Start_Session (NU_USB_STACK * stk,
                                     NU_USB_HW * hw,
                                     UINT8 port_id, UINT16 delay)
{
    STATUS         status;
    STATUS         internal_sts = NU_SUCCESS;
    INT            i;
    NU_USBH_STACK  *cb;
    UINT8          role;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(stk);
    NU_USB_PTRCHK_RETURN(hw);

    cb = (NU_USBH_STACK *)stk;
    status = _NU_USBH_STACK_Lock (stk);
    if (status == NU_SUCCESS)
    {
        status = NU_USB_NOT_SUPPORTED;
        for(i = 0; i < NU_USBH_MAX_HW; i++)
        {
            if(cb->bus_resources[i].controller == (NU_USBH_HW *)hw)
            {
                break;
            }
        }

        if(i != NU_USBH_MAX_HW)
        {
            status = NU_USB_HW_Get_Role(hw, port_id, &role);
            if(status == NU_SUCCESS)
            {
                status = NU_USB_INVLD_ARG;

                if((role & 0x02) == 0x02)
                {
                    status = NU_USB_HW_Start_Session(hw, port_id, delay);
                }
            }
        }
        internal_sts = _NU_USBH_STACK_Unlock (stk);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ((status == NU_SUCCESS) ? internal_sts : status);
}

/*************************************************************************
* FUNCTION
*       _NU_USBH_STACK_End_Session
*
* DESCRIPTION
*        This function is invoked by Applications when they wish to end a
*        session.
*
* INPUTS
*        'cb'      :   Stack control block.
*        'fc'      :   Hardware on which SRP is to be initiated
*        'port_id' :   port identifier on which SRP is to be initiated
*
* OUTPUTS
*       NU_SUCCESS              The service could be executed successfully.
*       NU_USB_INVLD_ARG        Specified port could not be found
*       NU_USB_NOT_SUPPORTED    This service is not supported by the FC
*
*************************************************************************/
STATUS _NU_USBH_STACK_End_Session (NU_USB_STACK * stk,
                                    NU_USB_HW * hw,
                                    UINT8 port_id)
{
    STATUS         status;
    STATUS         internal_sts = NU_SUCCESS;
    INT            i;
    NU_USBH_STACK  *cb;
    UINT8          role;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(stk);
    NU_USB_PTRCHK_RETURN(hw);

    cb = (NU_USBH_STACK *) stk;

    status = _NU_USBH_STACK_Lock (stk);

    if (status == NU_SUCCESS)
    {
        status = NU_USB_NOT_SUPPORTED;

        for(i = 0; i < NU_USBH_MAX_HW; i++)
        {
            if(cb->bus_resources[i].controller == (NU_USBH_HW *)hw)
            {
                break;
            }
        }

        if(i != NU_USBH_MAX_HW)
        {
            status = NU_USB_HW_Get_Role(hw, port_id, &role);

            if(status == NU_SUCCESS)
            {
                status = NU_USB_INVLD_ARG;

                if((role & 0x03) == 0x03)
                {
                    status = NU_USB_HW_End_Session(hw, port_id);
                }
            }
        }
        internal_sts = _NU_USBH_STACK_Unlock (stk);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ((status == NU_SUCCESS) ? internal_sts : status);
}

/*************************************************************************
* FUNCTION
*    NU_USBH_STACK_Get_Devices
*
* DESCRIPTION
*    returns the first non root-hub device attached to the host controller
*
* INPUTS
*    hw   pointer to the host controller control block
*
* OUTPUTS
*    pointer to the control block of the device attached
*    NU_NULL if there is no device attached
*
*************************************************************************/
NU_USB_DEVICE * NU_USBH_STACK_Get_Devices (NU_USBH_HW *hw)
{
    NU_USBH_STACK *cb;
    INT i;
    NU_USB_DEVICE *device = NU_NULL;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    if(hw == NU_NULL)
    {
        NU_USER_MODE();
        return (NU_NULL);
    }

    cb = (NU_USBH_STACK *)(((NU_USB_HW *)hw)->stack_cb);

    for(i = 0; i < NU_USBH_MAX_HW; i++)
    {
        if(cb->bus_resources[i].controller == hw)
        {
            /* find the device connected on this port */
           device = cb->bus_resources[i].dev_list;

           if ((device) && (device->function_address == USB_ROOT_HUB))
           {
               device = (NU_USB_DEVICE *)device->node.cs_next;
           }

           break;
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return device;
}

/*************************************************************************
* FUNCTION
*       NU_USBH_STACK_Switch_Config
*
* DESCRIPTION
*        This function is invoked by Applications when device configuration
*        is needed to be switched at run time.
* INPUTS
*       cb              Pointer to stack host control block.
*       dev             Pointer to device control block.
*       config_num      Configuration to be set.
*
*
* OUTPUTS
*       NU_SUCCESS              The service could be executed successfully.
*       NU_USB_INVLD_ARG        Parameters invalid.
*       NU_UNAVAILABLE          This configuration is not available.
*
*************************************************************************/
STATUS NU_USBH_STACK_Switch_Config(NU_USBH_STACK *stack,
                                  NU_USB_DEVICE *device,
                                  UINT8 config_num)
{
    STATUS              status      = NU_SUCCESS;
    STATUS              int_status  = NU_SUCCESS;
    INT                 configured  = NU_FALSE;
    INT                 initialized = 0;
    NU_USB_CFG          *config     = NU_NULL;
    NU_USB_DRVR         *vendor     = NU_NULL;
    NU_USB_ALT_SETTG    *alt        = NU_NULL;
    NU_USB_DRVR         *driver     = NU_NULL;
    USB_DRVR_LIST       *head       = NU_NULL;
    USB_DRVR_LIST       *list       = NU_NULL;
    USBH_BUS_RESOURCES  *bus        = NU_NULL;
    BOOLEAN              unlock     = NU_FALSE;
    INT  i, a;  /* configuration, interface, alternate setting index */

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Parameters validation. */
    NU_USB_PTRCHK_RETURN(stack);
    NU_USB_PTRCHK_RETURN(device);

    if ( config_num >= ((device->device_descriptor).bNumConfigurations))
    {
        /* Switch back to user mode. */
        NU_USER_MODE();
        return NU_USB_INVLD_ARG;
    }

    /* If the configuration is already set. */
    if (device->active_cnfg_num ==  config_num)
    {
        /* Switch back to user mode. */
        NU_USER_MODE();
        return (NU_SUCCESS);
    }

    /*Exclusive access to device. */
    status = NU_USB_DEVICE_Lock(device);

    /* Find the associated bus */
    if (status == NU_SUCCESS)
    {
        unlock = NU_TRUE;
        bus = usbh_find_bus (stack, device);
    }

    /* Invoke driver's disconnect function, if one exists.The driver
     *  disconnect function flushes the associated pipes.
     */

    if (bus != NU_NULL)
    {
        if (device->driver)
        {
            status = NU_USB_DRVR_Disconnect (device->driver,
                                            (NU_USB_STACK *) stack,
                                            (NU_USB_DEVICE *) device);
        }
        else
        {
          /* Detachment of device that has an associated driver */
            for (i = 0; i < NU_USB_MAX_INTERFACES; i++)
            {
                driver = device->config_descriptors[device->active_cnfg_num]->
                                        intf[i].driver;

                if (driver)
                {
                    status = NU_USB_DRVR_Disconnect (driver,
                                                    (NU_USB_STACK *) stack,
                                                    device);
                    if (status != NU_SUCCESS)
                    {
                        break;
                    }
                }
            }
        }

        /* Un-set the active configuration and close the associated pipes.*/
        if (status == NU_SUCCESS)
        {
            status = usb_unset_config(stack, device, bus);
        }

        /* Unlock the device here because it will be again locked when
         * new configuration will be set.
         */
        if (status == NU_SUCCESS)
        {
            unlock = NU_FALSE;
            status = NU_USB_DEVICE_Unlock(device);
        }

        if (status == NU_SUCCESS)
        {
            /* Find the associated configuration descriptor from the valid
             * configuration number.
             */
            config = device->config_descriptors[config_num];

            /* First search for the vendor specific driver.*/
            vendor = usb_find_next_best_vendor_drvr(stack,
                                        &(device->device_descriptor),
                                        stack->usb_stack.class_driver_list_head,
                                        0);
            if (vendor)
            {
                if (configured == NU_FALSE)
                {
                    /* The device should be configured */
                    status = usb_set_config(stack, device, bus, config);
                    if(status == NU_SUCCESS)
                    {
                        device->active_cnfg_num = config_num;
                        configured = NU_TRUE;
                    }
                }

                if((configured == NU_TRUE))
                {
                    /* Call the device driver initialization function. */
                    status = NU_USB_DRVR_Initialize_Device(vendor,
                                                (NU_USB_STACK *)stack, device);
                    if(status == NU_SUCCESS)
                    {
                        initialized++;
                    }
                }
            }

            else
            {
                /* Search for the best matching standard class driver. */
                for (i = 0; i < config->desc->bNumInterfaces; i++)
                {
                    head = stack->usb_stack.class_driver_list_head;
                    driver = NU_NULL;
                    list = NU_NULL;

                    while(head)
                    {
                        /* check each alternate setting for standard class
                         * driver.
                         */
                        for(a = 0; a < NU_USB_MAX_ALT_SETTINGS; a++)
                        {
                            alt = &(config->intf[i].alt_settg[a]);
                            if(alt->desc)
                            {
                                list = usb_find_next_best_std_driver(stack,
                                              alt->desc, head, 0);
                                if(list)
                                {
                                    driver = list->driver;
                                    break;
                                }
                                else
                                {
                                    driver = NU_NULL;
                                }
                            }
                        }

                        /* driver is found for non-configured device */
                        if((driver) && (configured == NU_FALSE))
                        {
                            /* the device should be configured */
                            status = usb_set_config(stack, device, bus, config);
                            if(status == NU_SUCCESS)
                            {
                                device->active_cnfg_num = config_num;
                                configured = NU_TRUE;
                            }
                        }

                        /* device is just configured or has multiple interfaces */
                        if((driver) && (configured == NU_TRUE))
                        {
                            /* call class driver for the interface */
                            status = NU_USB_DRVR_Initialize_Interface(driver,
                                                          (NU_USB_STACK *)stack,
                                                          device,
                                                          &config->intf[i]);
                            if(status == NU_SUCCESS)
                            {
                                initialized++;
                                break;   /* go to next interface */
                            }
                            else
                            {
                                /* search class driver list from the next driver */
                                head = ((USB_DRVR_LIST *)(list->list_node.cs_next));
                                if(head == stack->usb_stack.class_driver_list_head)
                                {
                                    head = NU_NULL;
                                }
                            }
                        }

                        if((driver == NU_NULL )|| (configured == NU_FALSE))
                        {
                            break;
                        }
                    } /* while(list) */
                } /* for (i = 0; .. */
            }
        }
    }

    /* Did drivers fail to initialize the device ?*/
    if ((configured == NU_TRUE) && (initialized == 0))
    {
        /* move the device back to addressed mode */
        int_status = usb_unset_config(stack, device, bus);
        configured = NU_FALSE;
    }

    if (unlock == NU_TRUE)
    {
        int_status |= NU_USB_DEVICE_Unlock(device);
    }

    if(configured == NU_TRUE)
    {
        status =  NU_SUCCESS;
    }
    else
    {
        status = NU_UNAVAILABLE;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ((status == NU_SUCCESS) ? int_status: status);
}

/*************************************************************************
* FUNCTION
*       NU_USBH_STACK_Get_Config_Info
*
* DESCRIPTION
*        This function returns the number of configurations
*        and there ID.
* INPUTS
*   dev             Pointer to device control block.
*   num_config      Pointer to hold number of configurations.
*   config_values   Pointer to hold number(ID) of each configuration
*                   according to host data structures.The size of parameter
*                   in bytes should be equal to or greater then the
*                   NU_USB_MAX_CONFIGURATIONS macro.
*
* OUTPUTS
*       NU_SUCCESS              The service could be executed successfully.
*       NU_USB_INVLD_ARG        Parameters invalid.
*
*************************************************************************/
STATUS NU_USBH_STACK_Get_Config_Info(NU_USB_DEVICE *device,
                                     UINT8 *num_config,
                                     UINT8 *config_values)
{
    STATUS status = NU_SUCCESS;
    UINT8  i;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(num_config);
    NU_USB_PTRCHK_RETURN(device);
    NU_USB_PTRCHK_RETURN(config_values);

    *num_config = (device->device_descriptor).bNumConfigurations;

    for ( i = 0 ; i < ((device->device_descriptor).bNumConfigurations) ; i++)
    {
        *(config_values + i) = i;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
/*************************************************************************
* FUNCTION
*        NU_USBH_STACK_U1_Enable
*
* DESCRIPTION
*       This function sends set_feature(U1_ENABLE) request to the
*       SuperSpeed device.The U1_ENABLE feature selector enables the
*       device to initiates U1 transition on the link when entry
*       conditions for U1 are met.For details regarding this request
*       please refer to the section 9.4.9 of the spec.
*
* INPUTS
*       cb             Pointer to stack control block.
*       device         Pointre to device control block
*
* OUTPUTS
*
*     NU_SUCCESS             Successful completion.
*     NU_USB_INVLD_ARG       Any of the input arguments is in invalid.
*     NU_USB_INVLD_DEVICE    Device is invalid.
*
*************************************************************************/
STATUS NU_USBH_STACK_U1_Enable(NU_USB_STACK  *cb,
                               NU_USB_DEVICE *device)
{
    STATUS        status;
    NU_USBH_STACK *stack;

    NU_SUPERV_USER_VARIABLES
    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(device);

    stack  = (NU_USBH_STACK *) cb;
    status = NU_USB_INVLD_DEVICE;

    /* Validate the device. */
    if (_NU_USBH_STACK_Is_Valid_Device ((NU_USB_STACK *) stack, device))
    {
        status = USBH_Modify_Device_Feature(stack, device,
                                            USB_SET_FEATURE,
                                            USB_FEATURE_U1_ENABLE);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*        NU_USBH_STACK_U1_Disable
*
* DESCRIPTION
*       This function sends clear_feature(U1_ENABLE) request to the
*       SuperSpeed device.The U1_ENABLE feature selector disables the
*       device from U1 transition initiation.For details regarding this
*       request please refer to the section 9.4.3 of the spec.
*
* INPUTS
*       cb             Pointer to stack control block.
*       device         Pointre to device control block
*
* OUTPUTS
*
*     NU_SUCCESS             Successful completion.
*     NU_USB_INVLD_ARG       Any of the input arguments is in invalid.
*     NU_USB_INVLD_DEVICE    Device is invalid.
*
*************************************************************************/
STATUS NU_USBH_STACK_U1_Disable(NU_USB_STACK  *cb,
                               NU_USB_DEVICE *device)
{
    STATUS        status;
    NU_USBH_STACK *stack;

    NU_SUPERV_USER_VARIABLES
    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(device);

    stack  = (NU_USBH_STACK *) cb;
    status = NU_USB_INVLD_DEVICE;

    /* Validate the device. */
    if (_NU_USBH_STACK_Is_Valid_Device ((NU_USB_STACK *) stack, device))
    {
        status = USBH_Modify_Device_Feature(stack, device,
                                            USB_CLEAR_FEATURE,
                                            USB_FEATURE_U1_ENABLE);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*        NU_USBH_STACK_U2_Enable
*
* DESCRIPTION
*       This function sends set_feature(U2_ENABLE) request to the
*       SuperSpeed device.The U2_ENABLE feature selector enables the
*       device to initiates U2 transition on the link when entry
*       conditions for U2 are met.For details regarding this request
*       please refer to the section 9.4.9 of the USB 3.0 specs.
*
*
* INPUTS
*       cb             Pointer to stack control block.
*       device         Pointer to device control block
*
* OUTPUTS
*
*     NU_SUCCESS             Successful completion.
*     NU_USB_INVLD_ARG       Any of the input arguments is in invalid.
*     NU_USB_INVLD_DEVICE    Device is invalid.
*
*************************************************************************/
STATUS NU_USBH_STACK_U2_Enable(NU_USB_STACK  *cb,
                               NU_USB_DEVICE *device)
{
    STATUS        status;
    NU_USBH_STACK *stack;

    NU_SUPERV_USER_VARIABLES
    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(device);

    stack  = (NU_USBH_STACK *) cb;
    status = NU_USB_INVLD_DEVICE;

    /* Validate the device. */
    if (_NU_USBH_STACK_Is_Valid_Device ((NU_USB_STACK *) stack, device))
    {
        status = USBH_Modify_Device_Feature(stack, device,
                                            USB_SET_FEATURE,
                                            USB_FEATURE_U2_ENABLE);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*        NU_USBH_STACK_U2_Disable
*
* DESCRIPTION
*       This function sends clear_feature(U2_ENABLE) request to the
*       SuperSpeed device.The U2_ENABLE feature selector disables the
*       device to initiate U2 transition . For details regarding this
*       request please refer to the section 9.4.3 of the spec.
*
* INPUTS
*       cb             Pointer to stack control block.
*       device         Pointre to device control block
*
* OUTPUTS
*
*     NU_SUCCESS             Successful completion.
*     NU_USB_INVLD_ARG       Any of the input arguments is in invalid.
*     NU_USB_INVLD_DEVICE    Device is invalid.
*
*************************************************************************/
STATUS NU_USBH_STACK_U2_Disable(NU_USB_STACK  *cb,
                               NU_USB_DEVICE *device)
{
    STATUS        status;
    NU_USBH_STACK *stack;

    NU_SUPERV_USER_VARIABLES
    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(device);

    stack  = (NU_USBH_STACK *) cb;
    status = NU_USB_INVLD_DEVICE;

    /* Validate the device. */
    if (_NU_USBH_STACK_Is_Valid_Device ((NU_USB_STACK *) stack, device))
    {
        status = USBH_Modify_Device_Feature(stack, device,
                                            USB_CLEAR_FEATURE,
                                            USB_FEATURE_U2_ENABLE);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*        NU_USBH_STACK_LTM_Enable
*
* DESCRIPTION
*       This function enables the LTM generation feature of the
*       the device.
*
* INPUTS
*       cb             ptr to stack control block.
*       device         ptr to device control block
*
* OUTPUTS
*
*     NU_SUCCESS             Successful completion.
*     NU_USB_INVLD_ARG       Any of the input arguments is in invalid.
*     NU_USB_TRANSFER_FAILED Transfer is not completed successfully.
*     NU_USB_INVLD_DEVICE    Device is invalid.
*
*************************************************************************/
STATUS NU_USBH_STACK_LTM_Enable(NU_USB_STACK *cb,
                               NU_USB_DEVICE *device)
{
    STATUS        status;
    NU_USBH_STACK *stack;

    NU_SUPERV_USER_VARIABLES
    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(device);

    stack  = (NU_USBH_STACK *) cb;
    status = NU_USB_INVLD_DEVICE;

    /* Validate the device. */
    if (_NU_USBH_STACK_Is_Valid_Device ((NU_USB_STACK *) stack, device))
    {
        status = USBH_Modify_Device_Feature(stack, device,
                                            USB_SET_FEATURE,
                                            USB_FEATURE_LTM_ENABLE);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( status );
}

/*************************************************************************
* FUNCTION
*        NU_USBH_STACK_LTM_Disable
*
* DESCRIPTION
*       This function disables the LTM generation feature of the
*       the device.
*
* INPUTS
*       cb             ptr to stack control block.
*       device         ptr to device control block
*
* OUTPUTS
*
*     NU_SUCCESS             Successful completion.
*     NU_USB_INVLD_ARG       Any of the input arguments is in invalid.
*     NU_USB_TRANSFER_FAILED Transfer is not completed successfully.
*     NU_USB_INVLD_DEVICE    Device is invalid.
*
*************************************************************************/
STATUS NU_USBH_STACK_LTM_Disable(NU_USB_STACK *cb,
                                 NU_USB_DEVICE *device)
{
    STATUS        status;
    NU_USBH_STACK *stack;

    NU_SUPERV_USER_VARIABLES
    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(device);

    stack  = (NU_USBH_STACK *) cb;
    status = NU_USB_INVLD_DEVICE;

    /* Validate the device. */
    if (_NU_USBH_STACK_Is_Valid_Device ((NU_USB_STACK *) stack, device))
    {
        status = USBH_Modify_Device_Feature(stack, device,
                                            USB_CLEAR_FEATURE,
                                            USB_FEATURE_LTM_ENABLE);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( status );
}

/*************************************************************************
* FUNCTION
*        _NU_USBH_STACK_Function_Suspend
*
* DESCRIPTION
*       This function sends set_feature(Function_Suspend) request to the
*       device.
*
* INPUTS
*       cb           Pointer to stack control block.
*       intf         Pointer to NU_USB_INTF control block.
*       func_suspend Boolean containing value of function suspend option.
*       rmt_wakeup   Boolean containing value of remote wakeup option.
*
* OUTPUTS
*
*     NU_SUCCESS             Successful completion.
*     NU_USB_INVLD_ARG       Any of the input arguments is in invalid.
*     NU_USB_INVLD_DEVICE    Device is invalid.
*
*************************************************************************/
STATUS _NU_USBH_STACK_Function_Suspend (NU_USB_STACK    *cb,
                                        NU_USB_INTF     *intf,
                                        BOOLEAN         func_suspend,
                                        BOOLEAN         rmt_wakeup)
{
    STATUS           status;
    NU_USBH_STACK    *stack;
    NU_USB_DEVICE    *dev;
    UINT8            options;
    UINT8            index;
    NU_USB_ALT_SETTG *current_stg;

    NU_SUPERV_USER_VARIABLES
    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(intf);

    stack   = (NU_USBH_STACK *) cb;
    dev     = intf->device;
    status  = NU_USB_INVLD_DEVICE;
    options = 0;

    if (_NU_USBH_STACK_Is_Valid_Device ((NU_USB_STACK *) stack, dev))
    {
        /* Disable the endpoints associated with the active alternate
         * setting of interface.
         */

        current_stg = intf->current;

        if (current_stg)
        {
            for ( index = 0; index < NU_USB_MAX_ENDPOINTS; index++ )
            {
                if (current_stg->endp[index].desc == NU_NULL)
                    continue;

                /* Invoke Controller's API */
                status = NU_USBH_HW_Disable_Pipe((NU_USBH_HW *) dev->hw,
                                                current_stg->endp[index].desc->bEndpointAddress,
                                                dev->function_address );
                if ( status != NU_SUCCESS )
                    break;
            }
        }

        if ( status == NU_SUCCESS )
        {
            /* Decode the func_suspend and rmt_wakeup input parameters. */
            options = (rmt_wakeup << 1) | func_suspend;

            /* Call the usb_modify_interface_feature function which will
             * initiate set_feature request.
             */
            status  =  USBH_Modify_Interface_Feature(stack, dev, intf,
                                                USB_SET_FEATURE, options,
                                                USB_FEATURE_FUNCTION_SUSPEND);
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( status );
}

#endif      /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */
/************************************************************************/

#endif /* USBH_STACK_EXT_C */
/* ======================  End Of File  =============================== */

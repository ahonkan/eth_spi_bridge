/**************************************************************************
*
*               Copyright 2004 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
* FILE NAME
*
*       nu_usbh_hid_ext.c
*
*
* COMPONENT
*
*       Nucleus USB Host HID Base Class Driver.
*
* DESCRIPTION
*
*       This file contains the external Interfaces exposed by Nucleus USB
*       Host HID Class Driver.
*
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_USBH_HID_Create                  Initializes the driver.
*       NU_USBH_HID_Get_Country_Code        Retrieves the country code.
*                                           supported by the HID device.
*       NU_USBH_HID_Get_Idle                Retrieves the current idle rate
*                                           for the HID device.
*       NU_USBH_HID_Get_Items               Returns the ITEMs belonging to
*                                           the device, supported by the
*                                           user.
*       NU_USBH_HID_Get_Report              Retrieves a specified report
*                                           from the device.
*       NU_USBH_HID_Set_Idle                Sets the idle rate for the
*                                           device.
*       NU_USBH_HID_Set_Report              Sets a report on the device.
*       _NU_USBH_HID_Delete                 Driver deletion callback.
*       _NU_USBH_HID_Disconnect             Device disconnection callback.
*       _NU_USBH_HID_Initialize_Intf        Device detection callback.
*       NU_USBH_HID_Init_GetHandle          This function is used to
*                                           retrieve address of the host
*                                           HID Class driver.
*
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/

/* ==============  USB Include Files =================================  */
#include    "connectivity/nu_usb.h"
#include    "services/runlevel_init.h"

/* ==========================  Functions ============================== */



/**************************************************************************
*
* FUNCTION
*
*       nu_os_conn_usb_host_hid_class_init
*
* DESCRIPTION
*
*       HID Driver initialization routine.
*
* INPUTS
*
*       path                                Registry path of component.
*       compctrl                           Flag to find if component is
*                                           being enabled or disabled.
*
* OUTPUTS
*
*       NU_SUCCESS                          Successful initialization.
*       NU_USB_INVLD_ARG                    Indicates that parameters are
*                                           NU_NULL.
*       NU_INVALID_POOL                     Indicates the supplied pool
*                                           pointer is invalid.
*       NU_INVALID_SIZE                     Indicates the size is larger
*                                           than the pool.
*       NU_NO_MEMORY                        Memory not available.
*       NU_USBH_MS_DUPLICATE_INIT           Indicates initialization error
*
**************************************************************************/
STATUS nu_os_conn_usb_host_hid_class_init(CHAR *path, INT compctrl)
{
    STATUS  status = NU_SUCCESS, internal_sts = NU_SUCCESS;
    UINT8   rollback = 0;
    NU_USB_STACK *stack_handle;
    VOID* usbh_hid_stack = NU_NULL;


    if (compctrl== RUNLEVEL_START)
    {
        /* Allocate memory for USB host HID class driver. */
        status = USB_Allocate_Object(sizeof(NU_USBH_HID),
                                     (VOID **)&NU_USBH_HID_Cb_Pt);
        if (status == NU_SUCCESS)
        {
            /* Zero out allocated block. */
            memset(NU_USBH_HID_Cb_Pt, 0, sizeof(NU_USBH_HID));

            /* Allocate memory for USB host HID task stack. */
            status = USB_Allocate_Memory(USB_MEM_TYPE_CACHED,
                                         USBH_HID_TASK_STACK_SIZE,
                                         (VOID **)&usbh_hid_stack);

            /* Create the device subsystem. */
            if(status == NU_SUCCESS)
            {
                status = NU_USBH_HID_Create (NU_USBH_HID_Cb_Pt, "USBH_HID",
                                            NU_NULL,
                                            usbh_hid_stack,
                                            USBH_HID_TASK_STACK_SIZE,
                                            USBH_HID_TASK_PRIORITY);

                /* Get the host stack handle */
                if(status == NU_SUCCESS)
                {
                    status = NU_USBH_Init_GetHandle ((void **)&stack_handle);

                    /* register to the stack */
                    if(status == NU_SUCCESS)
                    {
                        status = NU_USB_STACK_Register_Drvr ((NU_USB_STACK *) stack_handle,
                                                    (NU_USB_DRVR *) NU_USBH_HID_Cb_Pt);
                        if ( status != NU_SUCCESS )
                        {
                            rollback = 3;
                        }
                    }
                    else
                    {
                        rollback = 3;
                    }
                }
                else
                {
                    rollback = 2;
                }
            }
            else
            {
                rollback = 1;
            }
        }
    }
    else if(compctrl == RUNLEVEL_STOP)
    {
        internal_sts |= NU_USBH_Init_GetHandle ((VOID **)&stack_handle);
        internal_sts |= _NU_USB_STACK_Deregister_Drvr ((NU_USB_STACK *) stack_handle,
                                                       (NU_USB_DRVR *) NU_USBH_HID_Cb_Pt);
        rollback = 3;
    }

    switch(rollback)
    {
        case 3:
            internal_sts |= _NU_USBH_HID_Delete((void*)NU_USBH_HID_Cb_Pt);

        case 2:
            internal_sts |= USB_Deallocate_Memory(usbh_hid_stack);

        case 1:
            internal_sts |= USB_Deallocate_Memory(NU_USBH_HID_Cb_Pt);
        default:
            NU_UNUSED_PARAM(internal_sts);
    }

    return ( status );

}
/*************************************************************************
* FUNCTION
*
*       NU_USBH_HID_Create
*
* DESCRIPTION
*
*       HID Driver initialization routine
*
* INPUTS
*       cb                      Pointer to HID Driver control block.
*       name                    Name of this USB object.
*       pool                    Memory Pool used by the driver.
*       hid_task_stack_address  HID Task Stack.
*       hid_task_stack_size     HID Task Size.
*       hid_task_priority       HID Task priority.
*
*
* OUTPUTS
*
*       NU_SUCCESS            Successful Initialization.
*       NU_USB_INVLD_ARG      Indicates that the driver passed is NU_NULL
*                             or that the drvr->match_flag contains
*                             invalid values or that the
*                             drvr->initialize_device function pointer is
*                             with USB_MATCH_VNDR_ID field set in the
*                             drvr->match_flag.
*       NU_INVALID_SEMAPHORE  Indicates the semaphore pointer is invalid.
*       NU_SEMAPHORE_DELETED  Semaphore was deleted while the task
*                             was suspended.
*       NU_UNAVAILABLE        Indicates the semaphore is unavailable.
*       NU_INVALID_SUSPEND    Indicates that this API is called from
*                             a non-task thread.
*
*
*************************************************************************/
STATUS  NU_USBH_HID_Create ( NU_USBH_HID     *cb,
                             CHAR            *name,
                             NU_MEMORY_POOL  *pool,
                             VOID            *hid_task_stack_address,
                             UNSIGNED         hid_task_stack_size,
                             OPTION           hid_task_priority)
{
    STATUS status = NU_SUCCESS;
    INT rollback = 0;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    NU_USBH_HID_ASSERT(cb);
    NU_USBH_HID_ASSERT(name);
    NU_USBH_HID_ASSERT(hid_task_stack_address);
    NU_USB_MEMPOOLCHK_RETURN(pool);

    /* the Semaphores. */
    status = NU_Create_Semaphore (&(cb->intr_irp_complete),
                                  "HIDINTR", 0, NU_FIFO);
    if (status != NU_SUCCESS)
    {
        rollback = 1;
    }

    if(!rollback)
    {
        status = NU_Create_Semaphore (&(cb->ev_lock),
                                                    "HIDEVLK", 1, NU_FIFO);
        if (status != NU_SUCCESS)
        {
            rollback = 2;
        }
    }

    if(!rollback)
    {
        cb->pool = pool;
        memset (&(cb->task), 0, sizeof (NU_TASK));

        /* Base class behavior. */
        status = _NU_USBH_DRVR_Create ((NU_USB_DRVR *) cb, "USBH-HID",
                                       USB_MATCH_CLASS, 0,
                                       0, 0, 0, USB_HID_CLASS_CODE, 0, 0,
                                       &USBH_HID_Dispatch);
        if (NU_SUCCESS != status)
        {
            rollback = 3;
        }
    }

    if ( !rollback )
    {
        /* Create the task. */
        status = NU_Create_Task (&cb->task, "tHID", USBH_HID_Task,
                0, cb, hid_task_stack_address,
                hid_task_stack_size, hid_task_priority, 0,
                NU_PREEMPT, NU_START);
    }

    if (status != NU_SUCCESS)
    {
        rollback = 4;
    }

    switch(rollback)
    {
        case    4   : _NU_USBH_DRVR_Delete (cb);
        case    3   :  NU_Delete_Semaphore (&(cb->ev_lock));
        case    2   :  NU_Delete_Semaphore (&(cb->intr_irp_complete));
        default:
        break;
    }

    /* Return to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*
*        _NU_USBH_HID_Delete
*
* DESCRIPTION
*
*       This function deletes an HID driver. All Interfaces claimed
*       by this driver are given disconnect callback and the interfaces
*       are released. Driver is also unregistered from the stack it was
*       registered before deletion. Note that this function does not free
*       the memory associated with the HID Driver control block.
*
* INPUTS
*
*       cb              Driver control block.
*
* OUTPUTS
*
*       NU_SUCCESS      Indicates successful completion.
*
*
*************************************************************************/
STATUS  _NU_USBH_HID_Delete (VOID *cb)
{
    NU_USBH_HID *hidcb = (NU_USBH_HID *) cb;
    USBH_HID_DEVICE *next, *dev = hidcb->dev_list_head;
    STATUS status;

    NU_USBH_HID_ASSERT(cb);

    /* for each HID interface connected. */
    while (dev)
    {
        next = (USBH_HID_DEVICE*) dev->node.cs_next;

        /* and send disconnect event to each. */
        status = _NU_USBH_HID_Disconnect ((NU_USB_DRVR *) hidcb,
                                    dev->stack,
                                    dev->device);
        NU_USBH_HID_ASSERT( status == NU_SUCCESS );

        if ((next == hidcb->dev_list_head) ||
            (hidcb->dev_list_head == NU_NULL))
            dev = NU_NULL;
        else
            dev = next;
    }
    status = NU_Terminate_Task(&hidcb->task);
    NU_USBH_HID_ASSERT( status == NU_SUCCESS );

    status = NU_Delete_Semaphore (&(hidcb->ev_lock));
    NU_USBH_HID_ASSERT( status == NU_SUCCESS );

    status = NU_Delete_Semaphore (&(hidcb->intr_irp_complete));
    NU_USBH_HID_ASSERT( status == NU_SUCCESS );

    status = NU_Delete_Task(&hidcb->task);
    NU_USBH_HID_ASSERT( status == NU_SUCCESS );

    status = _NU_USBH_DRVR_Delete(cb);
    NU_USBH_HID_ASSERT( status == NU_SUCCESS );

    return (status);
}

/*************************************************************************
* FUNCTION
*       _NU_USBH_HID_Initialize_Intf
*
* DESCRIPTION
*
*       Connect Callback function invoked by stack when a Interface
*       with HID class is found on a device.
*
* INPUTS
*
*       cb          Pointer to Class Driver Control Block.
*       stk         Pointer to Stack Control Block of the calling stack.
*       dev         Pointer to Device Control Block of the device found.
*       intf        Pointer to Interface control Block to be served by
*                   this class driver.
*
* OUTPUTS
*
*       NU_SUCCESS              Indicates successful completion of the
*                               service.
*       NU_NOT_PRESENT          -Indicates No Alternate Setting with
*                               supported protocols is found.
*                               -Indicates No user associated with the
*                               subclass is found.
*                               -Indicates, Endpoints required by the
*                               protocol are not found.
*       NU_USB_INVLD_ARG        Indicates some control block(s) is(are)
*                               deleted before completion.
*       NU_NO_MEMORY            Indicates Memory Pool to allocate DEVICE
*                               structure is full.
*       NU_USB_INVLD_DESC       Indicates some descriptors of Device are
*                               incorrect.
*       NU_USB_MAX_EXCEEDED     Indicates Class Driver is already serving
*                               as many devices as it can support.
*       NU_INVALID_SUSPEND      Indicates call is made from a non thread
*                               context.
*
*
*************************************************************************/
STATUS  _NU_USBH_HID_Initialize_Intf ( NU_USB_DRVR   *cb,
                                       NU_USB_STACK  *stk,
                                       NU_USB_DEVICE *dev,
                                       NU_USB_INTF   *intf)
{
    /* new device connected. */
    NU_USBH_HID         *hidcb;
    USBH_HID_DEVICE     *hidDev;
    NU_USB_ALT_SETTG    *alt_setting;
    STATUS              status;
    UINT8               subclass, i, rollback;
    UINT8               alt_setting_num;
    UINT32              report_size;
    BOOLEAN             report_id_supported;

    NU_USBH_HID_ASSERT(cb);
    NU_USBH_HID_ASSERT(stk);
    NU_USBH_HID_ASSERT(dev);
    NU_USBH_HID_ASSERT(intf);

    status              = NU_SUCCESS;
    hidcb               = (NU_USBH_HID*)cb;
    hidDev              = NU_NULL;
    alt_setting_num     = 0;
    report_size         = 0;
    report_id_supported = NU_FALSE;
    rollback            = 0;

    do
    {
        /* Find an alternate setting on the device with Matching
         * Class Code.
         */
        status = NU_USB_INTF_Find_Alt_Setting ( intf,
                                                USB_MATCH_CLASS,
                                                alt_setting_num,
                                                USB_HID_CLASS_CODE, 0, 0,
                                                &alt_setting);
        if (status != NU_SUCCESS)
        {
            break;
        }

        /* Allocate a USBH_HID_DEVICE structure for connected device. This
         * structure is allocated for each Interface of this class connected
         * to the host.
         */
        status = USB_Allocate_Object (sizeof(USBH_HID_DEVICE),
                                      (VOID **) &hidDev);
        if (status != NU_SUCCESS)
        {
            break;
        }

        memset (hidDev, 0, sizeof (USBH_HID_DEVICE));

        /* Allocate memory for control transfers. */
        status = USB_Allocate_Memory (USB_MEM_TYPE_UNCACHED,
                                      8,
                                      (VOID **) &(hidDev->control_buffer));
        if (status != NU_SUCCESS)
        {
            rollback = 1;
            break;
        }

        memset(hidDev->control_buffer, 0, 8);

        /*
         * Device is initially considered disconnected.
         */
        hidDev->Connected = NU_FALSE;

        /* Find the default pipe. */
        status = NU_USB_ALT_SETTG_Find_Pipe (alt_setting,
                                        USB_MATCH_EP_ADDRESS, 0, 0,
                                        0, &hidDev->control_pipe);
        if (status != NU_SUCCESS)
        {
            rollback = 2;
            break;
        }

        /* Find the interrupt pipe.  */
        status = NU_USB_ALT_SETTG_Find_Pipe (alt_setting,
                                            USB_MATCH_EP_TYPE, 0, 0, 3,
                                             &hidDev->interrupt_pipe);

        if (status != NU_SUCCESS)
        {
            rollback = 2;
            break;
        }

        /* Set our alternate setting as the current alternate setting. */
        status = NU_USB_ALT_SETTG_Set_Active (alt_setting);
        if (status != NU_SUCCESS)
        {
            rollback = 2;
            break;
        }

        status = USBH_HID_Get_HID_Desc(hidcb, alt_setting, hidDev);
        if (status != NU_SUCCESS)
        {
            rollback = 2;
            break;
        }

        /* Fill in other information for DEVICE. */
        hidDev->drvr = cb;
        hidDev->stack = stk;
        hidDev->device = dev;
        hidDev->intf = intf;
        hidDev->alt_settg = alt_setting;

        status = NU_Create_Semaphore (&(hidDev->hid_lock),
                                                    "hid_lk", 1, NU_FIFO);

        if(status != NU_SUCCESS)
        {
            rollback = 2;
            break;
        }

        status = NU_Create_Semaphore (&(hidDev->cntrl_irp_complete),
                                                    "HIDCNTL", 0, NU_FIFO);
        if (status != NU_SUCCESS)
        {
            rollback = 3;
            break;
        }

        /* if the device is a default boot device, set it in report
         * mode.
         */
        status = NU_USB_ALT_SETTG_Get_SubClass(alt_setting, &subclass);
        if (status == NU_SUCCESS )
        {
            if(subclass)
            {
                status = USBH_HID_Set_Report_Protocol(hidcb, hidDev);
            }
        }

        if( status != NU_SUCCESS )
        {
            rollback = 4;
            break;
        }

        /* retrieve and parse the report descriptor. */
        status = USBH_HID_Get_Report_Desc(hidcb, hidDev);
        if ( status != NU_SUCCESS )
        {
            /* rollback is set to 4 because, there is a possiblity that
             * memory is allocated to 'items' and 'collection' even if
             * this function returns error.
             */
            rollback = 5;
            break;
        }

        status = USBH_HID_Connect_Users(hidcb, hidDev);
        if ( status != NU_SUCCESS )
        {
            rollback = 5;
            break;
        }

        for (i=0;i<USBH_HID_MAX_REPORT_IDS;i++)
        {
            if (hidDev->input_report_info[i].report_size > report_size)
            {
                report_size = hidDev->input_report_info[i].report_size;
                if (hidDev->input_report_info[i].report_id > 0)
                {
                    report_id_supported = NU_TRUE;
                }
            }
        }

        report_size = (report_size + hidDev->in_report_size + 7 )/8;
        if (report_id_supported)
        {
            /* add one byte space for report id */
            report_size++;
        }

        status = USB_Allocate_Memory (USB_MEM_TYPE_UNCACHED,
                                      report_size,
                                      (VOID **) &(hidDev->raw_report));
        if ( status != NU_SUCCESS )
        {
            rollback = 5;
            break;
        }

        memset (hidDev->raw_report, 0, report_size);

        /* Mark the Interface as claimed by this class driver. */
        status = NU_USB_INTF_Claim (intf, cb);
        if ( status != NU_SUCCESS )
        {
            rollback = 6;
            break;
        }

        /* Create IRP. */
        status = NU_USB_IRP_Create (&(hidDev->interrupt_irp),
            report_size,
            hidDev->raw_report,              /* Data. */
            1, 0, USBH_HID_Intr_Complete,    /* Callback function. */
            hidDev,                          /* Context. */
            0);
        if(status != NU_SUCCESS)
        {
            rollback = 7;
            break;
        }

        /* Submit the IRP. */
        status = NU_USB_PIPE_Submit_IRP (hidDev->interrupt_pipe,
                                (NU_USB_IRP *) &(hidDev->interrupt_irp));
        if ( status != NU_SUCCESS )
        {
            rollback = 7;
            break;
        }

        hidDev->Connected = NU_TRUE;

        /* Place USBH_HID_DEVICE Structure on the list. */
        NU_Place_On_List ((CS_NODE **) & hidcb->dev_list_head,
                (CS_NODE *) hidDev);
    }while(0);

    switch(rollback)
    {
        case 7: NU_USB_INTF_Release (intf, cb);
        case 6:
                USB_Deallocate_Memory(hidDev->raw_report);
                hidDev->raw_report = NU_NULL;
        case 5:
            for(i = 0; i < hidDev->num_items; i++)
            {
                if ( hidDev->usages[i].item )
                {
                    USB_Deallocate_Memory (hidDev->usages[i].item);
                    hidDev->usages[i].item = NU_NULL;
                }
            }

            for(i = 0; i < hidDev->num_of_coll; i++)
            {
                if ( hidDev->collection[i] )
                {
                    USB_Deallocate_Memory(hidDev->collection[i]);
                    hidDev->collection[i] = NU_NULL;
                }
            }
        case 4: NU_Delete_Semaphore(&(hidDev->cntrl_irp_complete));
        case 3: NU_Delete_Semaphore(&(hidDev->hid_lock));
        case 2: USB_Deallocate_Memory (hidDev->control_buffer);
        case 1: USB_Deallocate_Memory (hidDev); break;
        default: break;
    }

    return (status);
}

/*************************************************************************
* FUNCTION
*
*       _NU_USBH_HID_Disconnect
*
* DESCRIPTION
*
*       Disconnect Callback function, invoked by stack when an interface
*       with HID Class is removed from the BUS.
*
* INPUTS
*
*       cb      Pointer to Class Driver Control Block claimed this
*               Interface.
*       stk     Pointer to Stack Control Block.
*       dev     Pointer to NU_USB_DEVICE Control Block disconnected.
*
* OUTPUTS
*
*       NU_SUCCESS      Indicates successful completion of the service.
*
*
*************************************************************************/
STATUS  _NU_USBH_HID_Disconnect (NU_USB_DRVR     *cb,
                                 NU_USB_STACK    *stack,
                                 NU_USB_DEVICE   *device)
{
    NU_USBH_HID         *hidcb;
    USBH_HID_DEVICE     *next, *dev;
    INT                 num_users;
    NU_USBH_HID_USER    *cur_user;
    UINT8               i, j;
    STATUS              status = NU_SUCCESS;
    NU_USBH_HID_USER    *user_list[NU_USB_MAX_HID_USAGES] = { NU_NULL };

    NU_USBH_HID_ASSERT(cb);
    NU_USBH_HID_ASSERT(stack);
    NU_USBH_HID_ASSERT(device);

    hidcb      = (NU_USBH_HID*)cb;
    dev         = hidcb->dev_list_head;
    num_users   = 0;

    /* Scan the list of DEVICE's and cleanup all associated ones. */
    while (dev)
    {
        status = NU_Obtain_Semaphore(&(hidcb->ev_lock), NU_SUSPEND);
        if ( status == NU_SUCCESS )
        {
            next = (USBH_HID_DEVICE*) dev->node.cs_next;

            /* If this is associated with the device disconnected... */
            if ((dev->device == device) && (dev->Connected == NU_TRUE))
            {
                /* call user disconnect callbacks. */
                for (i = 0; i < dev->num_items; i++)
                {
                    cur_user = dev->usages[i].user;
                    if(cur_user == NU_NULL)
                    {
                        continue;
                    }

                    for(j = 0; j < num_users; j++)
                    {
                        if(user_list[j] == cur_user)
                        {
                            break;
                        }
                    }

                    if(j == num_users)
                    {
                        user_list[num_users++] = cur_user;
                        status = NU_USB_USER_Disconnect(
                                               (NU_USB_USER *)cur_user,
                                               cb,
                                               dev);

                        NU_USBH_HID_ASSERT( status == NU_SUCCESS );

                        /* Mark device as disconnected. */
                        dev->Connected = NU_FALSE;
                    }
                }

                /*
                 * De-allocate all resources if no interrupt irp is pending.
                 * otherwise cleanup will be performed when this irp is
                 * completed or canceled.
                 */
                if(dev->int_irp_completed == 1)
                {
                    NU_Obtain_Semaphore(&(dev->hid_lock),NU_SUSPEND);
                    /* Free allocated memory. */
                    if(dev->raw_report)
                    {
                        status = USB_Deallocate_Memory (dev->raw_report);
                        NU_USBH_HID_ASSERT( status == NU_SUCCESS );

                        dev->raw_report = NU_NULL;
                    }

                    for(i = 0; i < dev->num_items; i++)
                    {
                        if ( dev->usages[i].item )
                        {
                            status =
                                    USB_Deallocate_Memory (dev->usages[i].item);
                            NU_USBH_HID_ASSERT( status == NU_SUCCESS );

                            dev->usages[i].item = NU_NULL;
                        }
                    }

                    for(i = 0; i < dev->num_of_coll; i++)
                    {
                        if ( dev->collection[i] )
                        {
                            status = USB_Deallocate_Memory (dev->collection[i]);
                            NU_USBH_HID_ASSERT( status == NU_SUCCESS );

                            dev->collection[i] = NU_NULL;
                        }
                    }

                    status = NU_Delete_Semaphore (&(dev->cntrl_irp_complete));
                    NU_USBH_HID_ASSERT( status == NU_SUCCESS );

                    status = NU_Delete_Semaphore (&(dev->hid_lock));
                    NU_USBH_HID_ASSERT( status == NU_SUCCESS );

                    /* Remove the device Structure from the List... */
                    NU_Remove_From_List ((CS_NODE **) & hidcb->dev_list_head,
                                         (CS_NODE *) dev);

                    /* Deallocate buffer for control transfers. */
                    status = USB_Deallocate_Memory (dev->control_buffer);
                    NU_USBH_HID_ASSERT( status == NU_SUCCESS );

                    /* ...and Deallocate DEVICE structure. */
                    status = USB_Deallocate_Memory (dev);
                    NU_USBH_HID_ASSERT( status == NU_SUCCESS );
                }
            }

            if ((next == hidcb->dev_list_head) ||
                (hidcb->dev_list_head == NU_NULL))
            {
                dev = NU_NULL;
            }
            else
            {
                dev = next;
            }

            status = NU_Release_Semaphore (&(hidcb->ev_lock));
        }
    }

    return (status);
}

/*************************************************************************
* FUNCTION
*
*       NU_USBH_HID_Get_Country_Code
*
* DESCRIPTION
*
*       This function returns the country code specified by the HID device
*       in its report descriptor.
*
* INPUTS
*
*       cb                  Pointer to HID driver control block.
*       user                Pointer to user control block calling this API.
*       session             Pointer to device.
*       country_code_out    Location where the country_code is filled.
*
* OUTPUTS
*
*       NU_SUCCESS          Indicates successful completion.
*
*
*************************************************************************/
STATUS NU_USBH_HID_Get_Country_Code(NU_USBH_HID         *cb,
                                    NU_USBH_HID_USER    *user,
                                    VOID                *session,
                                    UINT8               *country_code_out)
{
    USBH_HID_DEVICE *hidDev;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    NU_USBH_HID_ASSERT(cb);
    NU_USBH_HID_ASSERT(user);
    NU_USBH_HID_ASSERT(session);
    NU_USBH_HID_ASSERT(country_code_out);

    hidDev = (USBH_HID_DEVICE *) session;

    *country_code_out = hidDev->hid_desc->bCountryCode;

    /* Return to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*
*       NU_USBH_HID_Get_Items
*
* DESCRIPTION
*
*       This function returns the ITEMs that belong to the HID device
*       identified by 'session'. Each user driver supports one or more
*       usage_page/usage_id combinations. Those items (in the device)
*       that have the 'user' supported usage_page/usage_id combinations
*       are returned to the user.
*
* INPUTS
*
*       cb              Pointer to HID driver control block.
*       user            Pointer to user control block calling this API.
*       session         Pointer to device.
*       items           Pointer to an array of 'item' structures, where
*                       the data will be filled in.
*       requested       Maximum number of items that can be returned.
*       avail_items_out Location where the actual number of items
*                       that are returned will be stored.
*
* OUTPUTS
*
*       NU_SUCCESS      Indicates successful completion.
*
*
*************************************************************************/
STATUS NU_USBH_HID_Get_Items(NU_USBH_HID        *cb,
                             NU_USBH_HID_USER   *user,
                             VOID               *session,
                             NU_USBH_HID_ITEM   *items,
                             UINT8               requested,
                             UINT8              *avail_items_out)
{
    UNSIGNED num, i, j;
    USBH_HID_DEVICE *hidDev = (USBH_HID_DEVICE *) session;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    NU_USBH_HID_ASSERT(cb);
    NU_USBH_HID_ASSERT(user);
    NU_USBH_HID_ASSERT(session);
    NU_USBH_HID_ASSERT(items);
    NU_USBH_HID_ASSERT(avail_items_out);

    if (hidDev->num_items > requested)
    {
        num = requested;
    }
    else
    {
        num = hidDev->num_items;
    }

    *avail_items_out = num;

    for (i = 0,j=0; i < hidDev->num_items; i++)
    {
        if (hidDev->usages[i].user == user)
        {
            items[j++] = *(hidDev->usages[i].item);
            if(j >= num)
            {
                break;
            }
        }
    }

    /* Return to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*
*   NU_USBH_HID_Set_Report
*
* DESCRIPTION
*
*       This function sends an HID SET_REPORT control transfer with the HID
*       device identified by 'session'.
*
* INPUTS
*
*       cb                Pointer to HID driver control block.
*       user              Pointer to user control block calling this API.
*       session           Pointer to device.
*       report_id         Id,identifying the report to send the
*                         SET_REPORT to.
*       report_type       Type of the report (in/out/feature).
*       report_length     Length of the report data.
*       report_data       Location from which data is sent to the device.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*       NU_USB_STALL_ERR   Indicates a stall returned for the transfer
*       NU_USB_INVLD_ARG   Indicates an error in completion of transfer.
*
*
*************************************************************************/
STATUS  NU_USBH_HID_Set_Report(NU_USBH_HID       *cb,
                               NU_USBH_HID_USER  *user,
                               VOID              *session,
                               UINT8              report_id,
                               UINT8              report_type,
                               UINT16             report_length,
                               UINT8             *report_data)
{
    USBH_HID_DEVICE *hid_device = (USBH_HID_DEVICE *) session;
    NU_USBH_CTRL_IRP *irp =  NU_NULL;
    STATUS irp_status = NU_SUCCESS, status = NU_SUCCESS, internal_sts = NU_SUCCESS;
    UINT8 intf_num, *data_ptr;
    UINT16 wValue = report_type;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    NU_USBH_HID_ASSERT(cb);
    NU_USBH_HID_ASSERT(user);
    NU_USBH_HID_ASSERT(session);
    NU_USBH_HID_ASSERT(report_data);

    /* Remove unused parameter warning. */
    NU_UNUSED_PARAM(internal_sts);

    status = NU_USB_INTF_Get_Intf_Num (hid_device->intf, &intf_num);

    if (status == NU_SUCCESS)
    {
		/* Allocate memory for the NU_USBH_CTRL_IRP structure */
    	status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,sizeof(NU_USBH_CTRL_IRP),(VOID**)&irp);
    }

    if ( status == NU_SUCCESS )
    {
        wValue = wValue << 8;
        wValue |= report_id;

        data_ptr = hid_device->control_buffer;
        memcpy(data_ptr, report_data, report_length);

        /* Form the control request. */
        status = NU_USBH_CTRL_IRP_Create (
                irp,
                data_ptr,                    /* data.               */
                USBH_HID_Cntrl_Complete,     /* callback function.  */
                hid_device,                  /* context.            */
                USBH_HID_BMREQTYPE_HID_SET,  /* bmRequestType.      */
                USBH_HID_BREQUEST_SET_REPORT,/* bRequest.           */
                HOST_2_LE16(wValue),         /* wValue.             */
                HOST_2_LE16(intf_num),       /* wIndex.             */
                HOST_2_LE16(report_length)); /* wLength.            */
    }

    if ( status == NU_SUCCESS )
    {
        status = NU_Obtain_Semaphore (&(hid_device->hid_lock), NU_SUSPEND);
        if ( status == NU_SUCCESS )
        {
            /* Submit the IRP.   */
            status = NU_USB_PIPE_Submit_IRP (hid_device->control_pipe,
                                            (NU_USB_IRP *)irp);

            /* Semaphore must be released, no matter what is the return
             * status of NU_USB_PIPE_Sumbit_IRP.
             */
            internal_sts = NU_Release_Semaphore (&(hid_device->hid_lock));
        }
    }

    if ( (status == NU_SUCCESS) && ( internal_sts == NU_SUCCESS) )
    {
        /* Wait for the the IRP to be complete.  */
        status = NU_Obtain_Semaphore (&(hid_device->cntrl_irp_complete),
                                        NU_SUSPEND);

        if ( status == NU_SUCCESS )
        {
            /* ...and return status. */
            status = NU_USB_IRP_Get_Status ((NU_USB_IRP*)irp,&irp_status);
        }

        if ( status == NU_SUCCESS )
        {
            status = irp_status;
        }
    }
	
    /* Deallocate memory for the NU_USBH_CTRL_IRP structure */
    if (irp != NU_NULL)
    {
    	internal_sts = USB_Deallocate_Memory(irp);
    }

    /* Return to user mode. */
    NU_USER_MODE();

    return (internal_sts == NU_SUCCESS ? status : internal_sts);
}

/*************************************************************************
* FUNCTION
*
*       NU_USBH_HID_Get_Report
*
* DESCRIPTION
*
*       This function sends a HID GET_REPORT control transfer with the HID
*       device identified by 'session'. The report data retrieved is
*       stored in the buffer provided by the caller.
*
* INPUTS
*
*       cb              Pointer to HID driver control block.
*       user            Pointer to user control block calling this API.
*       session         Pointer to device.
*       report_id       Id, identifying the report to send the
*                       GET_REPORT to.
*       report_type     Type of the report (in/out/feature).
*       report_length   Length of the report data.
*       report_data     Location where, data retrieved from the device is
*                       stored.
*
* OUTPUTS
*
*       NU_SUCCESS          Indicates successful completion.
*       NU_USB_STALL_ERR    Indicates a stall returned for the transfer
*       NU_USB_INVLD_ARG    Indicates an error in completion of transfer.
*
*
*************************************************************************/
STATUS NU_USBH_HID_Get_Report(NU_USBH_HID       *cb,
                              NU_USBH_HID_USER  *user,
                              VOID              *session,
                              UINT8              report_id,
                              UINT8              report_type,
                              UINT16             report_length,
                              UINT8             *report_data,
                              UINT32            *actual_length)
{
    USBH_HID_DEVICE *hid_device = (USBH_HID_DEVICE *) session;
    NU_USBH_CTRL_IRP *irp = NU_NULL;
    STATUS irp_status = NU_SUCCESS, status = NU_SUCCESS, internal_sts = NU_SUCCESS;
    UINT8 intf_num, *data_ptr=NU_NULL;
    UINT16 wValue = report_type;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    NU_USBH_HID_ASSERT(cb);
    NU_USBH_HID_ASSERT(user);
    NU_USBH_HID_ASSERT(session);
    NU_USBH_HID_ASSERT(report_data);

    status = NU_USB_INTF_Get_Intf_Num (hid_device->intf, &intf_num);

    if (status == NU_SUCCESS)
    {
		/* Allocate memory for the NU_USBH_CTRL_IRP structure */
    	status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,sizeof(NU_USBH_CTRL_IRP),(VOID**)&irp);
    }

    if ( status == NU_SUCCESS )
    {
        wValue = wValue << 8;
        wValue |= report_id;

        data_ptr = hid_device->control_buffer;
        memset(data_ptr, 0, report_length);

        /* Form the control request. */
        status = NU_USBH_CTRL_IRP_Create (
            irp,
            report_data,                    /* data. */
            USBH_HID_Cntrl_Complete,        /* callback function. */
            hid_device,                     /* context. */
            USBH_HID_BMREQTYPE_HID_GET,     /* bmRequestType. */
            USBH_HID_BREQUEST_GET_REPORT,   /* bRequest. */
            HOST_2_LE16(wValue),            /* wValue. */
            HOST_2_LE16(intf_num),          /* wIndex. */
            HOST_2_LE16(report_length));    /* wLength. */
    }

    if ( status == NU_SUCCESS )
    {
        status = NU_Obtain_Semaphore (&(hid_device->hid_lock), NU_SUSPEND);
        if ( status == NU_SUCCESS )
        {
            /* Submit the IRP.   */
            status = NU_USB_PIPE_Submit_IRP(hid_device->control_pipe,
                                                    (NU_USB_IRP *) irp);

            internal_sts = NU_Release_Semaphore (&(hid_device->hid_lock));
        }
    }

    if ( (status == NU_SUCCESS) && (internal_sts == NU_SUCCESS) )
    {
        /* Wait for the the IRP to be complete.  */
        status = NU_Obtain_Semaphore (&(hid_device->cntrl_irp_complete),
                                                              NU_SUSPEND);

        if ( status == NU_SUCCESS )
        {
            /* ...and return status. */
            status = NU_USB_IRP_Get_Status ((NU_USB_IRP*)irp,&irp_status);
        }

        if ( ( status == NU_SUCCESS ) && ( irp_status == NU_SUCCESS ) )
        {
            status = NU_USB_IRP_Get_Actual_Length((NU_USB_IRP *)irp,
                                                            actual_length);
        }

        if ( status == NU_SUCCESS )
        {
            status = irp_status;
            memcpy(report_data, data_ptr, report_length);
        }
    }

    /* Deallocate memory for the NU_USBH_CTRL_IRP structure */
    if (irp != NU_NULL)
    {
    	internal_sts = USB_Deallocate_Memory(irp);
    }

    /* Return to user mode. */
    NU_USER_MODE();

    return (internal_sts == NU_SUCCESS ? status : internal_sts);
}

/*************************************************************************
* FUNCTION
*
*       NU_USBH_HID_Set_Idle
*
* DESCRIPTION
*
*       This function sets the idle for the 'report' generated by the HID
*       device, identified by 'session'.
*
* INPUTS
*
*       cb          Pointer to HID driver control block.
*       user        Pointer to user control block calling this API.
*       session     Pointer to device.
*       report_id   Id, identifying the report to set the idle rate
*       idle_rate   Idle rate to be set.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*       NU_USB_STALL_ERR   Indicates a stall returned for the transfer
*       NU_USB_INVLD_ARG   Indicates an error in completion of transfer.
*
*************************************************************************/
STATUS NU_USBH_HID_Set_Idle(NU_USBH_HID         *cb,
                            NU_USBH_HID_USER    *user,
                            VOID                *session,
                            UINT8                report_id,
                            UINT8                idle_rate)
{
    USBH_HID_DEVICE *hidDev = (USBH_HID_DEVICE *) session;
    NU_USBH_CTRL_IRP *irp = NU_NULL;
    STATUS irp_status = NU_SUCCESS, status = NU_SUCCESS, internal_sts = NU_SUCCESS;
    UINT8 intf_num;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    NU_USBH_HID_ASSERT(cb);
    NU_USBH_HID_ASSERT(user);
    NU_USBH_HID_ASSERT(session);

    status = NU_USB_INTF_Get_Intf_Num (hidDev->intf, &intf_num);

    if (status == NU_SUCCESS)
    {
		/* Allocate memory for the NU_USBH_CTRL_IRP structure */
    	status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,sizeof(NU_USBH_CTRL_IRP),(VOID**)&irp);
    }

    if ( status == NU_SUCCESS )
    {
        /* Form the control request. */
        status = NU_USBH_CTRL_IRP_Create (irp,
                NU_NULL,                    /* data. */
                USBH_HID_Cntrl_Complete,    /* callback function.*/
                hidDev,                     /* context. */
                USBH_HID_BMREQTYPE_HID_SET, /* bmRequestType. */
                USBH_HID_BREQUEST_SET_IDLE, /* bRequest SET_IDLE.*/
                HOST_2_LE16(((UINT16)idle_rate<<0x08) | report_id),/* wValue. */
                HOST_2_LE16 (intf_num),     /* wIndex. */
                0);                         /* wLength. */
    }

    if ( status == NU_SUCCESS )
    {
        status = NU_Obtain_Semaphore (&(hidDev->hid_lock), NU_SUSPEND);
        if ( status == NU_SUCCESS )
        {
            /* Submit the IRP.   */
            status = NU_USB_PIPE_Submit_IRP (hidDev->control_pipe,
                                                    (NU_USB_IRP *) irp);

            internal_sts = NU_Release_Semaphore (&(hidDev->hid_lock));
        }
    }

    if ( (status == NU_SUCCESS) && (internal_sts == NU_SUCCESS) )
    {
        /* Wait for the the IRP to be complete.  */
        status = NU_Obtain_Semaphore (&(hidDev->cntrl_irp_complete),
                                                            NU_SUSPEND);

        if ( status == NU_SUCCESS )
        {
            /* ...and return status. */
            status = NU_USB_IRP_Get_Status ((NU_USB_IRP*)irp,&irp_status);
        }

        if ( status == NU_SUCCESS )
        {
            status = irp_status;
        }
    }

    /* Deallocate memory for the NU_USBH_CTRL_IRP structure */
    if (irp != NU_NULL)
    {
    	internal_sts = USB_Deallocate_Memory(irp);
    }

    /* Return to user mode. */
    NU_USER_MODE();

    return (internal_sts == NU_SUCCESS ? status : internal_sts);
}

/*************************************************************************
* FUNCTION
*
*       NU_USBH_HID_Get_Idle
*
* DESCRIPTION
*
*       This function retrieves the idle rate for the 'report' generated
*       by the HID device, identified by 'session'.
*
* INPUTS
*
*       cb              Pointer to HID driver control block.
*       user            Pointer to user control block calling this API.
*       session         Pointer to device.
*       report_id       Id, identifying the report to get the idle rate
*       idle_rate_out   Location where the idle rate is to be filled.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*       NU_USB_STALL_ERR   Indicates a stall returned for the transfer
*       NU_USB_INVLD_ARG   Indicates an error in completion of transfer.
*
*
*************************************************************************/
STATUS NU_USBH_HID_Get_Idle(NU_USBH_HID         *cb,
                            NU_USBH_HID_USER    *user,
                            VOID                *session,
                            UINT8                report_id,
                            UINT8               *idle_rate_out)
{
    USBH_HID_DEVICE *hidDev = (USBH_HID_DEVICE *) session;
    NU_USBH_CTRL_IRP *irp = NU_NULL;
    STATUS irp_status = NU_SUCCESS, status = NU_SUCCESS, internal_sts = NU_SUCCESS;
    UINT8 intf_num, *data_ptr=NU_NULL;
    UINT32 idle_rate = 0;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    NU_USBH_HID_ASSERT(cb);
    NU_USBH_HID_ASSERT(user);
    NU_USBH_HID_ASSERT(session);
    NU_USBH_HID_ASSERT(idle_rate_out);

    status = NU_USB_INTF_Get_Intf_Num (hidDev->intf, &intf_num);

    if (status == NU_SUCCESS)
    {
		/* Allocate memory for the NU_USBH_CTRL_IRP structure */
    	status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,sizeof(NU_USBH_CTRL_IRP),(VOID**)&irp);
    }

    if ( status == NU_SUCCESS )
    {
        data_ptr = hidDev->control_buffer;
        memset(data_ptr, 0, sizeof(idle_rate));

        /* Form the control request. */
        status = NU_USBH_CTRL_IRP_Create (irp,
                data_ptr,                   /* Data. */
                USBH_HID_Cntrl_Complete,    /* Callback function. */
                hidDev,                     /* Context. */
                USBH_HID_BMREQTYPE_HID_GET, /* bmRequestType. */
                USBH_HID_BREQUEST_GET_IDLE, /* bRequest SET_IDLE. */
                HOST_2_LE16(report_id),     /* wValue. */
                HOST_2_LE16 (intf_num),     /* wIndex. */
                HOST_2_LE16 (1));           /* wLength. */
    }

    if ( status == NU_SUCCESS )
    {
        status = NU_Obtain_Semaphore (&(hidDev->hid_lock), NU_SUSPEND);
        if ( status == NU_SUCCESS )
        {
            /* Submit the IRP.   */
            status = NU_USB_PIPE_Submit_IRP (hidDev->control_pipe,
                                                (NU_USB_IRP *) irp);

            internal_sts = NU_Release_Semaphore (&(hidDev->hid_lock));
        }
    }

    if ( (status == NU_SUCCESS) && (internal_sts == NU_SUCCESS) )
    {
        /* Wait for the the IRP to be complete.  */
        status = NU_Obtain_Semaphore (&(hidDev->cntrl_irp_complete),
                                                            NU_SUSPEND);

        if ( status == NU_SUCCESS )
        {
            /* ...and return status. */
            status = NU_USB_IRP_Get_Status ((NU_USB_IRP*)irp,&irp_status);
        }

        if ( status == NU_SUCCESS )
        {
            if(irp_status == NU_SUCCESS)
            {
                memcpy((UINT8 *)&idle_rate, data_ptr, sizeof(idle_rate));
                *idle_rate_out = (UINT8)(idle_rate & 0x00ff);
            }
            status = irp_status;
        }
    }

    /* Deallocate memory for the NU_USBH_CTRL_IRP structure */
    if (irp != NU_NULL)
    {
    	internal_sts = USB_Deallocate_Memory(irp);
    }

    /* Return to user mode. */
    NU_USER_MODE();

    return (internal_sts == NU_SUCCESS ? status : internal_sts);
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBH_HID_Init_GetHandle
*
*   DESCRIPTION
*
*       This function is called to retrieve the host HID
*       class driver's address.
*
*   INPUTS
*
*       handle              Double pointer used to retrieve the class
*                           driver's address.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicates there exists a host HID
*                           class driver.
*       NU_NOT_PRESENT      Indicates there exists no class driver.
*
*************************************************************************/
STATUS NU_USBH_HID_Init_GetHandle(VOID  **handle)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();

    NU_USBH_HID_ASSERT(handle);

    status = NU_SUCCESS;
    *handle = NU_USBH_HID_Cb_Pt;

    if (NU_USBH_HID_Cb_Pt == NU_NULL)
    {
        status = NU_NOT_PRESENT;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/* ======================  End Of File  =============================== */


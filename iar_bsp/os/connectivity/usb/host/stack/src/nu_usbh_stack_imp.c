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
*       nu_usbh_stack_imp.c
*
* COMPONENT
*       Nucleus USB Host Stack
*
* DESCRIPTION
*       Implementation of USB Host Stack APIs.
*
* DATA STRUCTURES
*       None
*
* FUNCTIONS
*       USBH_Is_Device_Self_Powered          Returns device's self powered
*                                            status.
*       USBH_Is_Device_Remote_Wakeup         Returns device remote wakeup
*                                            status.
*       USBH_Enumerate_Device                Enumerates a device.
*       USBH_Deenumerate_Device              De-enumerates a device.
*       USBH_Get_Parent                      Get parent from a device
*                                            control block.
*       USBH_Get_Port_Number                 Get port number of parent hub
*                                            for this device.
*       usb_hisr                             HISR of USB host controllers.
*       usb_scan_host_controllers            Entry function of the task
*                                            USBHStk.
*       usb_callback_hndlr                   Callback function USBD's
*                                            control transfers.
*       usb_control_transfer                 Submits a control IRP transfer.
*       usb_get_configuration                Fetches-n-parses device's
*                                            descriptors.
*       power                                Calculates a raised to b.
*       usb_interval_2_usecs                 Translates the bInterval value
*                                            of the endpoint descriptor.
*       usb_close_pipes                      Closes all the opened pipes of
*                                            the specified device.
*       usb_set_interface_id                 Sets the alternate setting.
*       usb_find_next_best_vendor_drvr       Finds best matching vendor
*                                            driver.
*       usb_find_next_best_std_driver        Finds best matching standard
*                                            driver.
*       usb_attempt_unclaimed_devs           Attempts the new driver on the
*                                            enumerated but un-attached
*                                            devices/interfaces.
*       usb_any_claimed_device               Checks to see if any device is
*                                            currently using the specified
*                                            driver.
*       usb_calc_load                        Calculates the b/w
*                                            requirements of an endpoint.
*       usb_set_config                       Sets the specified
*                                            configuration.
*       usb_unset_config                     Un-configures the specified
*                                            device.
*       usb_modify_ep                        Sends Endpoint Halt Clear/Set.
*       usb_verify_pipe                      Looks up in the device's
*                                            endpoint descriptor database.
*       USBH_Get_BOS_Descriptor              This function is used to fetch
*                                            BOS descriptor.
*       USBH_Set_Isochronous_Delay           This function sets isoch
*                                            delay during superspeed
*                                            device enumeration.
*       USBH_Set_System_Exit_Latency         This functionn sets SEL
*                                            during superspeed device
*                                            enumeration.
*       USBH_Modify_Interface_Feature        This function sends
*                                            set/cleare_feature()function
*                                            suspend request to the
*                                            SuperSpeed device
*       USBH_Modify_Device_Feature           This function sends
*                                            set/cleare_feature request to
*                                            the SuperSpeed device for
*                                            device level features.
*       usbh_test_mode_enable_using_pid      This function enables appropriate
*                                            host test mode that is based
*                                            on PID of the device attached to it.
*       usbh_test_mode_get_descriptor        This function is used during
*                                            testing of device in which
*                                            single step descriptor fetch is
*                                            tested.
*       usbh_test_mode_enumerate             This function is used to enumerate
*                                            device in test mode for executing
*                                            any of the tests
*
* DEPENDENCIES
*       nu_usb.h                             All USB definitions.
*
************************************************************************/
#ifndef USBH_STACK_IMP_C
#define USBH_STACK_IMP_C

/* ==============  Standard Include Files ============================  */
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "connectivity/nu_usb.h"

/*************************************************************************
* FUNCTION
*        USBH_Is_Device_Self_Powered
*
* DESCRIPTION
*    Gets the device status to know if the device is self powered.
*
* INPUTS
*  stack        ptr to stack control block.
*  device       ptr to device control block.
*  reply_out    on successful return, it contains 1 or 0 indicating the self
*               power nature of the device.
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
STATUS USBH_Is_Device_Self_Powered (NU_USBH_STACK * stack,
                                    NU_USB_DEVICE * device,
                                    BOOLEAN * reply_out)
{
    STATUS ret;
    UINT16 status;

    if(stack == NU_NULL
       || device == NU_NULL
       || reply_out == NU_NULL)
    {
        return NU_USB_INVLD_ARG;
    }

    ret = _NU_USBH_STACK_Get_Device_Status ((NU_USB_STACK *) stack,
                                          (NU_USB_DEVICE *) device, &status);
    if (ret == NU_SUCCESS)
        *reply_out = (BOOLEAN)(status & (~0xfffe));

    return (ret);
}

/*************************************************************************
* FUNCTION
*        USBH_Is_Device_Remote_Wakeup
*
* DESCRIPTION
*     Gets the device status to know if the device is remote wake up capable.
*
* INPUTS
*  stack        ptr to stack control block.
*  device       ptr to device control block.
*  reply_out    on successful return, it contains 1 or 0 indicating the
*               remote wake up nature of the device.
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
STATUS USBH_Is_Device_Remote_Wakeup (NU_USBH_STACK * stack,
                                     NU_USB_DEVICE * device,
                                     BOOLEAN * reply_out)
{
    STATUS ret;
    UINT16 status;

    if(stack == NU_NULL
       || device == NU_NULL
       || reply_out == NU_NULL)
    {
        return NU_USB_INVLD_ARG;
    }

    ret = _NU_USBH_STACK_Get_Device_Status ((NU_USB_STACK *) stack,
                                          (NU_USB_DEVICE *) device, &status);
    if (ret == NU_SUCCESS)
        *reply_out = (BOOLEAN)(status & (~0xfffd));
    return (ret);
}

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
/*************************************************************************
*
* FUNCTION
*       USBH_Enumerate_Device
*
* DESCRIPTION
*       Enumerates the newly detected USB device, identifies a best-fit
*       driver for the new device and sets a suitable configuration on the
*       device. This API is always invoked by the hub class driver from
*       "USBHHUB" context for all non root hubs. In case of root hub this
*       is called from NU_USB_Add_HW API.
*
* INPUTS
*       stack                                ptr to stack control block.
*       parent                               ptr to device control block
*                                            that detected the new device.
*       port_number                          port number on the parent
*                                            device on which this new
*                                            device is detected.
*                                            For root hub, this argument is
*                                            ignored.
*       speed                                USB speed USB_SPEED_FULL or
*                                            USB_SPEED_HIGH or
*                                            USB_SPEED_LOW.
*                                            Root Hubs cannot have
*                                            USB_SPEED_LOW.
*       new_device                           On successful return this
*                                            would contain a pointer to the
*                                            new device control block that
*                                            was created by this function.
* OUTPUTS
*
*       NU_SUCCESS                           Indicates successful
*                                            completion of the service.
*       NU_USB_INVLD_DESC                    Indicates that the descriptors
*                                            reported by the device contain
*                                            invalid values.
*       NU_INVALID_SEMAPHORE                 Indicates the semaphore
*                                            pointer is invalid.
*       NU_SEMAPHORE_DELETED                 Semaphore was deleted while
*                                            the task was suspended.
*       NU_UNAVAILABLE                       Indicates the semaphore is
*                                            unavailable.
*       NU_INVALID_POOL                      Indicates the dynamic memory
*                                            pool is invalid.
*       NU_INVALID_POINTER                   Indicates the return pointer
*                                            is NU_NULL.
*       NU_INVALID_SUSPEND                   Indicates that this API is
*                                            called from a non-task thread.
*       NU_USB_INVLD_ARG                     Indicates that the speed
*                                            passed to this API is an
*                                            invalid value or one of input
*                                            parameter is NU_NULL.
*       NU_NO_MEMORY                         Indicates failure of memory
*                                            allocation.
*       NU_USB_INVLD_SPEED                   Speed is invalid.
*
*************************************************************************/
STATUS USBH_Enumerate_Device  (NU_USBH_STACK  *stack,
                               NU_USB_DEVICE  *parent,
                               UINT8          port_number,
                               UINT8          speed,
                               NU_USB_DEVICE  **new_device)
{
    STATUS                        status           = NU_SUCCESS;
    STATUS                        internal_sts     = NU_SUCCESS;
    USBH_BUS_RESOURCES            *bus             = NU_NULL;
    NU_USB_DEVCAP_SUPERSPEED_DESC *ss_desc_out     = NU_NULL;
    NU_USB_PIPE                   *pipe            = NU_NULL;
    NU_USB_DEVICE                 *device          = NU_NULL;
    UINT8                         *temp_buffer     = NU_NULL;
    UINT32                        len              = 0;
    UINT8                         rollback         = 1;
    UINT8                         function_address = 0;
    UINT8                         root_hub         = 0;
    UINT8                         i                = 0;
    BOOLEAN                       ss_supported     = 0;
    BOOLEAN                       func_supported   = 0;

    /* Parameters Validation. */
    NU_USB_PTRCHK(stack);
    NU_USB_PTRCHK(parent);
    NU_USB_PTRCHK(new_device);

    /*  Initialize memory pointed to by new_device to NU_NULL to prevent
     *  errant value from being set in calling function
     */
    *new_device = NU_NULL;

    if ( (parent->function_address == 0) && (parent->parent == NU_NULL) )
    {
        /* Root hub address. */
        root_hub = 1;
    }

    if ( (speed > USB_SPEED_SUPER) || (speed == USB_SPEED_UNKNOWN) )
    {
        /* Report status that attached device is not supported. */
        internal_sts = NU_USB_STACK_OTG_Report_Status(
            (NU_USB_STACK *)stack, NU_USB_UNSUPPORTED_DEVICE);

        NU_UNUSED_PARAM(internal_sts);
        return ( NU_USB_INVLD_ARG );
    }

    /* Find the associated bus */
    bus = usbh_find_bus (stack, parent);

    if(bus == NU_NULL)
    {
        return ( NU_USB_INVLD_ARG );
    }

    /* Allocate Memory for the new device */
    /* Don't wait for memory, fail the enumeration, if memory is
     * not found.
     */
    status = USB_Allocate_Object(sizeof (NU_USB_DEVICE),
                                 (VOID **) &device);
    if ( status == NU_SUCCESS )
    {
        memset ((UINT8 *) device, 0, sizeof (NU_USB_DEVICE));

        device->stack = (NU_USB_STACK *) stack;
        device->hw = parent->hw;
        device->speed = speed;

        /*  If non-root  hub is under enumeration, assign it
         *  the default address to start with.
         */
        if (root_hub)
        {
            device->parent = NU_NULL;
            bus->root_hub = device;
        }
        else
        {
            device->parent = parent;
            device->port_number = port_number;
        }

        /*  If non-root hub is under enumeration, assign it
         *  the default address to start with.
         */
        if (root_hub)
        {
            device->function_address = USB_ROOT_HUB;
        }
        else
        {
            device->function_address = USB_DEFAULT_DEV_ID;
        }

        /* Build the default pipe */
        pipe = &device->ctrl_pipe;
        pipe->device = (NU_USB_DEVICE *) device;
        pipe->endpoint = NU_NULL;

        if (!root_hub)
        {
            status = NU_USB_HW_Open_Pipe((NU_USB_HW *) bus->controller,
                                          device->function_address,
                                          0x80,
                                          USB_EP_CTRL, 0, 0, 0, 0 );

        }

        if ( status == NU_SUCCESS )
        {
            status = NU_Create_Semaphore( &(device->lock), "DEVLOCK",
                                          1, NU_PRIORITY );
            if ( status == NU_SUCCESS )
            {
                if ( !root_hub )
                {
                    /* Not a Root Hub, so set an address */
                    function_address = usb_get_id (bus->dev_ids, USB_MAX_DEVID/8, 0);
                    if ( function_address != USB_NO_ID )
                    {
                        status = _NU_USBH_STACK_Lock ((NU_USB_STACK *) stack);
                        if ( status == NU_SUCCESS )
                        {
                            NU_Place_On_List ((CS_NODE **) & (bus->dev_list),
                                              (CS_NODE *) device);

                            status = _NU_USBH_STACK_Unlock ((NU_USB_STACK *) stack);
                            if ( status == NU_SUCCESS )
                            {
                                /* set the new address */
                                internal_sts |= USBH_FILL_CNTRLPKT ((stack->ctrl_irp),
                                            (USB_REQ_RECP_DEV | USB_REQ_STD |
                                             USB_REQ_OUT), USB_SET_ADDRESS,
                                             function_address, 0, 0);

                                len = usb_control_transfer (stack, pipe, stack->ctrl_irp,
                                                            NU_NULL, 0);
                                if ( len )
                                {
                                    /* delay for ensuring recovery interval USB 2.0 spec 9.2.6.3 */
                                    usb_wait_ms(2);
                                    device->function_address = function_address;
                                    device->state = USB_STATE_ADDRESSED;
                                }
                                else
                                {
                                    /* Failed to set the new address at the device */
                                    status = NU_USB_INVLD_DESC;
                                    rollback = 6;
                                }
                            }
                            else
                                rollback = 6;
                        }
                        else
                            rollback = 5;
                    }
                    else
                    {
                        status = NU_USB_NO_ADDR;
                        rollback = 5;
                    }
                }
                else
                {
                    /* Insert the Root hub in to the database of the HC */
                    device->function_address = USB_ROOT_HUB;
                    status = _NU_USBH_STACK_Lock ((NU_USB_STACK *) stack);
                    if ( status == NU_SUCCESS )
                    {
                        NU_Place_On_List ((CS_NODE **) & (bus->dev_list),
                                           (CS_NODE *) device);

                        status = _NU_USBH_STACK_Unlock ((NU_USB_STACK *) stack);
                        if ( status != NU_SUCCESS )
                        {
                            rollback = 6;
                        }
                    }
                    else
                        rollback = 5;
                }
            }
            else
                rollback = 4;
        }
        else
            rollback = 3;
    }
    else
        rollback = 2;


    if ( status == NU_SUCCESS )
    {
        /* Allocate uncached memory buffer for data IO. */
        status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                     sizeof (NU_USB_DEVICE_DESC),
                                     (VOID **) &temp_buffer);

        if(status == NU_SUCCESS)
        {
            /* Get first 8 bytes of Device Descriptor to know ep[0] size. */
            internal_sts = USBH_FILL_CNTRLPKT ( (stack->ctrl_irp),
                                                (USB_REQ_RECP_DEV | USB_REQ_STD | USB_REQ_IN),
                                                USB_GET_DESCRIPTOR, (USB_DT_DEVICE << 8), 0, 8);

            /* Zero out first 8 bytes of data IO buffer. */
            memset(temp_buffer, 0, 8);

            len = usb_control_transfer ( stack, pipe, stack->ctrl_irp,
                                         temp_buffer, 8 );
            if ( len == 8 )
            {
                /* Copy values into device control block. */
                memcpy(&(device->device_descriptor), temp_buffer, 8);

                /* Convert to host's native format. */
                device->device_descriptor.bcdDevice =
                    LE16_2_HOST (device->device_descriptor.bcdDevice);
                device->device_descriptor.idVendor =
                    LE16_2_HOST (device->device_descriptor.idVendor);
                device->device_descriptor.idProduct =
                    LE16_2_HOST (device->device_descriptor.idProduct);
                device->device_descriptor.bcdUSB =
                    LE16_2_HOST (device->device_descriptor.bcdUSB);

                if ( (device->device_descriptor.bcdUSB != 0x100)
                     && (device->device_descriptor.bcdUSB != 0x110)
                     && (device->device_descriptor.bcdUSB != 0x101)
                     && (device->device_descriptor.bcdUSB != 0x200)
                     && (device->device_descriptor.bcdUSB != 0x300)
                     && (device->device_descriptor.bcdUSB != 0x210) )
                {
                    status = NU_USB_INVLD_DESC;
                    rollback = 6;
                }
                else
                {
                    if ((device->device_descriptor.bMaxPacketSize0 != 8)
                        && (device->device_descriptor.bMaxPacketSize0 != 16)
                        && (device->device_descriptor.bMaxPacketSize0 != 32)
                        && (device->device_descriptor.bMaxPacketSize0 != 64)
                        && (device->device_descriptor.bMaxPacketSize0 != 0x09)
                        )
                    {
                        status = NU_USB_INVLD_DESC;
                        rollback = 6;
                    }
                }
            }
            else
            {
                /* Failed to fetch initial 8 bytes of the device descriptor. */
                status = NU_USB_INVLD_DESC;
                rollback = 6;
            }
        }
        else
        {
            rollback = 6;
        }
    }

    if ( status == NU_SUCCESS )
    {
        if ( ( speed < USB_SPEED_HIGH ) && (!root_hub) )
        {
            /* Update the default endpoint Max packet size.*/
            status = NU_USBH_HW_Update_Max_Packet_Size( bus->controller,
                                                        device,
                                                        device->device_descriptor.bMaxPacketSize0 );
            if ( status != NU_SUCCESS )
            {
                rollback = 6;
            }
        }
        if ( status == NU_SUCCESS )
        {
            /* Request entire device descriptor */
            internal_sts |= USBH_FILL_CNTRLPKT ((stack->ctrl_irp),
                            (USB_REQ_RECP_DEV | USB_REQ_STD | USB_REQ_IN),
                            USB_GET_DESCRIPTOR, (USB_DT_DEVICE << 8), 0,
                            sizeof (NU_USB_DEVICE_DESC));

            /* Zero out data IO buffer. */
            memset(temp_buffer, 0, sizeof (NU_USB_DEVICE_DESC));

            len = usb_control_transfer (stack, pipe, stack->ctrl_irp,
                                    temp_buffer,
                                    sizeof (NU_USB_DEVICE_DESC));
            if( len )
            {
                /* Copy values into device control block. */
                memcpy((VOID *)&(device->device_descriptor), (VOID *) temp_buffer, sizeof (NU_USB_DEVICE_DESC));

                /* Convert to CPU's native endian format.*/
                device->device_descriptor.bcdDevice =
                    LE16_2_HOST (device->device_descriptor.bcdDevice);
                device->device_descriptor.idVendor =
                    LE16_2_HOST (device->device_descriptor.idVendor);
                device->device_descriptor.idProduct =
                    LE16_2_HOST (device->device_descriptor.idProduct);
                device->device_descriptor.bcdUSB =
                    LE16_2_HOST (device->device_descriptor.bcdUSB);
            }
            else
            {
                /* Failed to fetch the device descriptor, discard the device. */
                status = NU_USB_INVLD_DESC;
            }
            if ( status != NU_SUCCESS )
            {
                rollback = 6;
            }
        }
    }

    if ( status == NU_SUCCESS )
    {
        /* Fetch all the string descriptors from the device */
        if (device->device_descriptor.iManufacturer)
        {
            status = usb_fetch_string_desc (stack, device,
                                   device->device_descriptor.iManufacturer);
        }
        if ( status == NU_SUCCESS )
        {
            if (device->device_descriptor.iProduct)
            {
                status = usb_fetch_string_desc (stack, device,
                                       device->device_descriptor.iProduct);
            }
            if ( status == NU_SUCCESS )
            {
                if (device->device_descriptor.iSerialNumber)
                {
                    status = usb_fetch_string_desc (stack, device,
                                    device->device_descriptor.iSerialNumber);
                }
            }
        }
        if ( status != NU_SUCCESS )
            rollback = 7;
    }

    /* USB 3.0 requests handling. */
    if ( status ==  NU_SUCCESS )
    {
        if ((device->device_descriptor.bcdUSB == 0x0300)
             &&(device->device_descriptor.bMaxPacketSize0 == 0x09) )
        {
            /* The conditions for device to be superspeed are met,
             * now proceed with the USB 3.0 standard requests.
             */
            /* Get the BOS descriptor. */
            status = USBH_Get_BOS_Descriptor(stack, device);
            if ( status == NU_SUCCESS )
            {
                /* Get the SuperSpeed device capability descriptor
                 * handle.
                 */
                status = NU_USB_DEVICE_Get_SuprSpd_Desc( device, &ss_desc_out );
                if ( status == NU_SUCCESS )
                {
                    /* Compare the speed found during link training to the
                     * speed reported in the SuperSpeed device capability
                     * descriptor.
                     */
                    status = NU_USB_DEVCAP_SuprSpd_Get_SS( device->bos, &ss_supported );
                    if ( status == NU_SUCCESS )
                    {
                        if ( (ss_supported && (speed == USB_SPEED_SUPER)) )
                        {
                            status = NU_USB_DEVCAP_SuprSpd_Get_Functionality( device->bos,
                                                                              &func_supported );
                            if ( (status == NU_SUCCESS) && (speed > func_supported) )
                            {
                                if ( !root_hub )
                                {
                                    status = USBH_Initialize_PM_interface(stack,
                                                                          bus->controller,
                                                                          device );
                                }
                            }
                        }
                        else
                        {
                            status = NU_USB_INVLD_SPEED;
                        }
                    }
                }
            }
            if ( status != NU_SUCCESS )
                rollback = 8;
        }
    }

    if ( status == NU_SUCCESS )
    {
        /* Now get complete configuration descriptor(s) and parse
         * each of them.
         */
        status = usb_get_configuration (stack, device);
        if ( status == NU_SUCCESS )
        {
            /* see if there is a vendor specific driver for this device */
            if(usb_bind_vendor_driver(stack, device,
                                      stack->usb_stack.class_driver_list_head, 0))
            {
                /* if so, standard class drivers will be skipped */
                *new_device = device;
            }
            else
            {
                /* find standard class driver for this device */
                status = usb_bind_standard_driver(stack, device, bus);
                if ( status == NU_SUCCESS )
                {
                    *new_device = device;
                }
                else
                {

                    /* is this an OTG test device? */
                    if (!((device->device_descriptor.idVendor == NU_USB_OTG_TEST_DEV_VID) &&
                          (device->device_descriptor.idProduct == NU_USB_OTG_TEST_DEV_PID)) )
                    {
                        /* if not, this is an unsupported device */
                        internal_sts |= NU_USB_STACK_OTG_Report_Status((NU_USB_STACK *)stack,
                                                       NU_USB_UNSUPPORTED_DEVICE);
                    }

                    /* no class driver for this device, device will be suspended */
                    internal_sts |= NU_USBH_STACK_Suspend_Device (stack, device);
                }
            }
        }
        if ( status != NU_SUCCESS )
            rollback = 9;
    }

    /* Clean up in case of error. */
    switch (rollback)
    {
        case 9:
           for (i = 0; i < device->device_descriptor.bNumConfigurations; i++)
            {
                if (device->config_descriptors[i])
                {
                    internal_sts |= USB_Deallocate_Memory (
                                            device->config_descriptors[i]);
                    device->config_descriptors[i] = NU_NULL;
                }

                if (device->raw_descriptors[i][device->speed])
                {
                    internal_sts |= USB_Deallocate_Memory (
                               device->raw_descriptors[i][device->speed]);
                    device->raw_descriptors[i][device->speed] = NU_NULL;
                }
            }

        case 8:
            if (device->bos)
            {
               internal_sts = USB_Deallocate_Memory (device->bos);
               device->bos = NU_NULL;
            }

            if(device->raw_bos_descriptors)
            {
               internal_sts = USB_Deallocate_Memory (
                                              device->raw_bos_descriptors);
               device->raw_bos_descriptors = NU_NULL;
            }

        case 7:
            for (i = 0; i < NU_USB_MAX_STRINGS; i++)
            {
                if (device->string_descriptors[i])
                {
                    internal_sts |= USB_Deallocate_Memory (
                                            device->string_descriptors[i]);
                    device->string_descriptors[i] = NU_NULL;
                }
            }

        case 6:
            NU_Remove_From_List ((CS_NODE **) & (bus->dev_list),
                                  (CS_NODE *) device);
            if (parent)
                usb_release_id (bus->dev_ids, USB_MAX_DEVID / 8,
                                function_address);

        case 5:
            internal_sts |= NU_Delete_Semaphore(&(device->lock));

        case 4:
            internal_sts |= NU_USB_HW_Close_Pipe ((NU_USB_HW *) bus->
                                  controller, device->function_address, 0x80);

        case 3:
            internal_sts |= USB_Deallocate_Memory (device);

        case 2:
            /* Report status that attached device is not supported. */
            internal_sts |= NU_USB_STACK_OTG_Report_Status((NU_USB_STACK *)stack,
                                            NU_USB_UNSUPPORTED_DEVICE);

        case 1:
            /* De-allocate temporary buffer now if allocated above. */
            if(temp_buffer != NU_NULL)
            {
                internal_sts |= USB_Deallocate_Memory(temp_buffer);
            }

            NU_UNUSED_PARAM(internal_sts);
            return (status);
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*       USBH_Initialize_PM_interface
*
* DESCRIPTION
*       Enumerates the newly detected USB device, identifies a best-fit
*       driver for the new device and sets a suitable configuration on the
*       device. This API is always invoked by the hub class driver from
*       "USBHHUB" context for all non root hubs. In case of root hub this
*       is called from NU_USB_Add_HW API.
*
* INPUTS
*       stack                                ptr to stack control block.
*       hw_ctrl                              ptr to USBH HW control block
*                                            that detected the new device.
*       device                               ptr to new connected device.
*
* OUTPUTS
*
*       NU_SUCCESS                           Indicates successful
*
*************************************************************************/
STATUS USBH_Initialize_PM_interface(NU_USBH_STACK *stack,
                                    NU_USBH_HW    *hw_ctrl,
                                    NU_USB_DEVICE *device)
{
    STATUS             status            = NU_SUCCESS;
    NU_USBH_DEVICE_HUB *hub              = NU_NULL;
    UINT8               latency          = 0;
    UINT8               bDeviceClass_out = 0;

    status = USBH_Set_Isochronous_Delay(stack, device);

    if ( status == NU_SUCCESS )
    {
       status = USBH_Set_System_Exit_Latency( stack, device );

        if ( status == NU_SUCCESS )
        {
            status = NU_USBH_HW_Update_Exit_Latency( hw_ctrl, stack, device ,&latency);

            if ( status == NU_SUCCESS )
            {
                /* Get the pointer to hub,where device is attached. This is required for
                 * setting hub downstream port inactivity timer values.
                 */
                hub = (NU_USBH_DEVICE_HUB *) usb_find_hub( &(stack->hub_driver), device->parent );

                /* hub = NULL - True for the root hub only.*/
                if ( (hub != NU_NULL) && (latency != 0x00) )
                {
                    /* Set the value of downstream port U1 inactivity timer. */
                    status = usbh_hub_port_set_timeout( stack, hub, device->port_number,
                                                        USBH_HUB_FEATURE_PORT_U1_TIMEOUT,
                                                        HUB_PORT_U1_TIMEOUT_VALUE );

                    if ( status == NU_SUCCESS )
                    {
                        status = NU_USB_DEVICE_Get_bDeviceClass(device, &bDeviceClass_out);
                        if ( status == NU_SUCCESS )
                        {
                            /* U2 timeout value should not be set for the downstream port
                             * connected to hub.
                             */
                            if ( bDeviceClass_out != USB_HUB_CLASS_CODE )
                            {
                                status = usbh_hub_port_set_timeout( stack, hub, device->port_number,
                                                                    USBH_HUB_FEATURE_PORT_U2_TIMEOUT,
                                                                    HUB_PORT_U2_TIMEOUT_VALUE );
                            }
                        }
                    }
                }
            }
        }
    }

    return ( status );
}

#else /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */

/*************************************************************************
*
* FUNCTION
*       USBH_Enumerate_Device (For Specs v2.0)
*
* DESCRIPTION
*       Enumerates the newly detected USB device, identifies a best-fit
*       driver for the new device and sets a suitable configuration on the
*       device. This API is always invoked by the hub class driver from
*       "USBHHUB" context for all non root hubs. In case of root hub this
*       is called from NU_USB_Add_HW API.
*
* INPUTS
*       stack                                ptr to stack control block.
*       parent                               ptr to device control block
*                                            that detected the new device.
*       port_number                          port number on the parent
*                                            device on which this new
*                                            device is detected.
*                                            For root hub, this argument is
*                                            ignored.
*       speed                                USB speed USB_SPEED_FULL or
*                                            USB_SPEED_HIGH or
*                                            USB_SPEED_LOW.
*                                            Root Hubs cannot have
*                                            USB_SPEED_LOW.
*       new_device                           On successful return this
*                                            would contain a pointer to the
*                                            new device control block that
*                                            was created by this function.
* OUTPUTS
*
*       NU_SUCCESS                           Indicates successful
*                                            completion of the service.
*       NU_USB_INVLD_DESC                    Indicates that the descriptors
*                                            reported by the device contain
*                                            invalid values.
*       NU_INVALID_SEMAPHORE                 Indicates the semaphore
*                                            pointer is invalid.
*       NU_SEMAPHORE_DELETED                 Semaphore was deleted while
*                                            the task was suspended.
*       NU_UNAVAILABLE                       Indicates the semaphore is
*                                            unavailable.
*       NU_INVALID_POOL                      Indicates the dynamic memory
*                                            pool is invalid.
*       NU_INVALID_POINTER                   Indicates the return pointer
*                                            is NU_NULL.
*       NU_INVALID_SUSPEND                   Indicates that this API is
*                                            called from a non-task thread.
*       NU_USB_INVLD_ARG                     Indicates that the speed
*                                            passed to this API is an
*                                            invalid value or one of input
*                                            parameter is NU_NULL.
*       NU_NO_MEMORY                         Indicates failure of memory
*                                            allocation.
*       NU_USB_INVLD_SPEED                   Speed is invalid.
*
*************************************************************************/
STATUS USBH_Enumerate_Device (NU_USBH_STACK * stack,
                              NU_USB_DEVICE * parent,
                              UINT8 port_number,
                              UINT8 speed,
                              NU_USB_DEVICE ** new_device)
{
    STATUS              status;
    STATUS              internal_sts = NU_SUCCESS;
    USBH_BUS_RESOURCES  *bus;
    UINT32              len = 0;
    UINT8               rollback = 0;
    NU_USB_PIPE         *pipe = NU_NULL;
    NU_USB_DEVICE       *device = NU_NULL;
    UINT8               *temp_buffer = NU_NULL;
    UINT8               function_address = 0, root_hub = 0;
    UINT8               old_address,i=0;

    if(stack == NU_NULL
       || parent == NU_NULL
       || new_device == NU_NULL)
    {
        return NU_USB_INVLD_ARG;
    }

    /*  Initialize memory pointed to by new_device to NU_NULL to prevent
     *  errant value from being set in calling function
     */

    *new_device = NU_NULL;

    if ((parent->function_address == 0) && (parent->parent == NU_NULL))
        root_hub = 1;

    if ((speed > USB_SPEED_HIGH) || (speed == USB_SPEED_UNKNOWN))
    {
        /* Report status that attached device is not supported. */
        internal_sts = NU_USB_STACK_OTG_Report_Status(
            (NU_USB_STACK *)stack, NU_USB_UNSUPPORTED_DEVICE);

        NU_UNUSED_PARAM(internal_sts);
        return NU_USB_INVLD_ARG;
    }

    /* Find the associated bus */
    bus = usbh_find_bus (stack, parent);

    if(bus == NU_NULL)
    {
        return NU_USB_INVLD_ARG;
    }
    /* Allocate Memory for the new device */
    /* Don't wait for memory, fail the enumeration, if memory is
     * not found.
     */
    status = USB_Allocate_Object(sizeof (NU_USB_DEVICE), (VOID **) &device);
    if (status != NU_SUCCESS)
    {
        rollback = 2;
    }

    if (!rollback)
    {
        memset ((UINT8 *) device, 0, sizeof (NU_USB_DEVICE));

        device->stack = (NU_USB_STACK *) stack;
        device->hw = parent->hw;
        device->speed = speed;

        /*  If non-root  hub is under enumeration, assign it
         *  the default address to start with.
         */
        if (root_hub)
        {
            device->parent = NU_NULL;
            bus->root_hub = device;
        }
        else
        {
            device->parent = parent;
            device->port_number = port_number;
        }

        /*  If non-root  hub is under enumeration, assign it
         *  the default address to start with.
         */
        if (root_hub)
        {
            device->function_address = USB_ROOT_HUB;
        }
        else
        {
            device->function_address = USB_DEFAULT_DEV_ID;
        }

        /* Build the default pipe */
        pipe = &device->ctrl_pipe;
        pipe->device = (NU_USB_DEVICE *) device;
        pipe->endpoint = NU_NULL;

        if (!root_hub)
        {
            /* Open the default pipe */
            status = NU_USB_HW_Open_Pipe ((NU_USB_HW *) bus->controller,
                                          device->function_address,
                                          0x80 /* default bEndpointAddress */ ,
                                          USB_EP_CTRL, speed, 8,
                                          0, 0);   /* interval and load */

            if (status != NU_SUCCESS)
                rollback = 3;
        }

        if (!rollback)
        {
            status = NU_Create_Semaphore(&(device->lock), "DEVLOCK",
                                         1, NU_PRIORITY);
            if (status != NU_SUCCESS)
            {
                rollback = 4;
            }
        }
    }

    if (!rollback)
    {
        /* Allocate uncached memory buffer for data IO. */
        status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                     sizeof (NU_USB_DEVICE_DESC),
                                     (VOID **) &temp_buffer);

        if(status != NU_SUCCESS)
        {
            rollback = 5;
        }

        if(!rollback)
        {
            /* Get first 8 bytes of Device Descriptor to know ep[0] size */
            internal_sts = USBH_FILL_CNTRLPKT ((stack->ctrl_irp),
                                (USB_REQ_RECP_DEV | USB_REQ_STD | USB_REQ_IN),
                                USB_GET_DESCRIPTOR, (USB_DT_DEVICE << 8), 0, 8);

            /* Zero out first 8 bytes of data IO buffer. */
            memset(temp_buffer, 0, 8);

            len = usb_control_transfer (stack, pipe, stack->ctrl_irp,
                                        temp_buffer, 8);
            if (len < 8)
            {
                /* Failed to fetch initial 8 bytes of the device descriptor */
                status = NU_USB_INVLD_DESC;
                rollback = 5;
            }
        }
    }

    if (!rollback)
    {
        /* Copy values into device control block. */
        memcpy(&(device->device_descriptor), temp_buffer, 8);

        /* Convert to host's native format */
        device->device_descriptor.bcdDevice =
            LE16_2_HOST (device->device_descriptor.bcdDevice);
        device->device_descriptor.idVendor =
            LE16_2_HOST (device->device_descriptor.idVendor);
        device->device_descriptor.idProduct =
            LE16_2_HOST (device->device_descriptor.idProduct);
        device->device_descriptor.bcdUSB =
            LE16_2_HOST (device->device_descriptor.bcdUSB);

        /* UnKnown USB Version  */
        if ((device->device_descriptor.bcdUSB != 0x100) &&
            (device->device_descriptor.bcdUSB != 0x110) &&
            (device->device_descriptor.bcdUSB != 0x101) &&
            (device->device_descriptor.bcdUSB != 0x200) &&
            (device->device_descriptor.bcdUSB != 0x210))
        {
            status = NU_USB_INVLD_DESC;
            rollback = 5;
        }

        if (!rollback)
        {
            if ((device->device_descriptor.bMaxPacketSize0 != 8)
                && (device->device_descriptor.bMaxPacketSize0 != 16)
                && (device->device_descriptor.bMaxPacketSize0 != 32)
                && (device->device_descriptor.bMaxPacketSize0 != 64))
            {
                status = NU_USB_INVLD_DESC;
                rollback = 5;
            }
        }

        if ((!rollback) && (!root_hub))
        {
            /* Not a Root Hub, so set an address */
            function_address = usb_get_id (bus->dev_ids, USB_MAX_DEVID/8, 0);

            if (function_address == USB_NO_ID)
            {
                status = NU_USB_NO_ADDR;
                rollback = 5;
            }
            if (!rollback)
            {
                bus->last_dev_id = function_address;

                status = _NU_USBH_STACK_Lock ((NU_USB_STACK *) stack);
                if (status != NU_SUCCESS)
                {
                    rollback = 5;
                }
                if(!rollback)
                {
                    NU_Place_On_List ((CS_NODE **) & (bus->dev_list),
                                   (CS_NODE *) device);

                    status = _NU_USBH_STACK_Unlock ((NU_USB_STACK *) stack);
                    if(status != NU_SUCCESS)
                    {
                        rollback = 6;
                    }
                }
                if(!rollback)
                {
                    /* set the new address */
                    internal_sts |= USBH_FILL_CNTRLPKT ((stack->ctrl_irp),
                                        (USB_REQ_RECP_DEV | USB_REQ_STD |
                                         USB_REQ_OUT), USB_SET_ADDRESS,
                                         function_address, 0, 0);

                    len = usb_control_transfer (stack, pipe, stack->ctrl_irp,
                                                  NU_NULL, 0);
                    if (!len)
                    {
                        /* Failed to set the new address at the device */
                        status = NU_USB_INVLD_DESC;
                        rollback = 6;
                    }
                }
                if (!rollback)
                {
                    /* delay for ensuring recovery interval USB 2.0 spec 9.2.6.3 */
                    usb_wait_ms(2);

                    old_address = device->function_address;
                    device->function_address = function_address;

                    device->state = USB_STATE_ADDRESSED;

                    /* close default pipe in the device with old address */
                    status = NU_USB_HW_Close_Pipe ((NU_USB_HW *) bus->controller,
                                          old_address,
                                          0x80 /* the default bEndpointAddress */ );

                    if (status != NU_SUCCESS)
                    {
                        rollback = 6;
                    }
                }

                if (!rollback )
                {
                    /* open default pipe in the device with new address */
                    status = NU_USB_HW_Open_Pipe ((NU_USB_HW *) bus->controller,
                                     device->function_address,
                                     0x80, USB_EP_CTRL, speed,
                                     device->device_descriptor.bMaxPacketSize0,
                                     0, 0);
                    if (status != NU_SUCCESS)
                    {
                        rollback = 6;
                    }
                }
            }
        }
        else if(!rollback)
        {
            /* Insert the Root hub in to the database of the HC */
            device->function_address = USB_ROOT_HUB;
            status = _NU_USBH_STACK_Lock ((NU_USB_STACK *) stack);
            if (status != NU_SUCCESS)
            {
                rollback = 5;
            }
            if(!rollback)
            {
                NU_Place_On_List ((CS_NODE **) & (bus->dev_list),
                               (CS_NODE *) device);

                status = _NU_USBH_STACK_Unlock ((NU_USB_STACK *) stack);
                if(status != NU_SUCCESS)
                {
                    rollback = 6;
                }
            }
        }

        if (!rollback)
        {
            /* request entire device descriptor */
            internal_sts |= USBH_FILL_CNTRLPKT ((stack->ctrl_irp),
                            (USB_REQ_RECP_DEV | USB_REQ_STD | USB_REQ_IN),
                            USB_GET_DESCRIPTOR, (USB_DT_DEVICE << 8), 0,
                            sizeof (NU_USB_DEVICE_DESC));

            /* Zero out data IO buffer. */
            memset(temp_buffer, 0, sizeof (NU_USB_DEVICE_DESC));

            len = usb_control_transfer (stack, pipe, stack->ctrl_irp,
                                    temp_buffer,
                                    sizeof (NU_USB_DEVICE_DESC));
            if (!len)
            {
                /* Failed to fetch the device descriptor, discard the device */
                status = NU_USB_INVLD_DESC;
                rollback = 7;
            }
        }

        if (!rollback)
        {
            /* Copy values into device control block. */
            memcpy(&(device->device_descriptor), temp_buffer, sizeof (NU_USB_DEVICE_DESC));

            /* convert to CPU's native endian format.
             */
            device->device_descriptor.bcdDevice =
                LE16_2_HOST (device->device_descriptor.bcdDevice);
            device->device_descriptor.idVendor =
                LE16_2_HOST (device->device_descriptor.idVendor);
            device->device_descriptor.idProduct =
                LE16_2_HOST (device->device_descriptor.idProduct);
            device->device_descriptor.bcdUSB =
                LE16_2_HOST (device->device_descriptor.bcdUSB);
        }
    }

    if (!rollback)
    {
        /* Fetch all the string descriptors from the device */
        if (device->device_descriptor.iManufacturer)
            status = usb_fetch_string_desc (stack, device,
                                   device->device_descriptor.iManufacturer);
        if (status != NU_SUCCESS)
        {
            rollback = 8;
        }

        if (device->device_descriptor.iProduct)
            status = usb_fetch_string_desc (stack, device,
                                   device->device_descriptor.iProduct);

        if (status != NU_SUCCESS)
        {
            rollback = 8;
        }

        if (device->device_descriptor.iSerialNumber)
            status = usb_fetch_string_desc (stack, device,
                                   device->device_descriptor.iSerialNumber);

        if (status != NU_SUCCESS)
        {
            rollback = 8;
        }

        if (!rollback)
        {
            /* Now get complete configuration descriptor(s) and parse
             * each of them
             */
            status = usb_get_configuration (stack, device);
            if (status != NU_SUCCESS)
            {
                /* Parsing or fetching of descriptors failed, discard the device */
                rollback = 8;
            }
        }
    }

    if (!rollback)
    {
        /* see if there is a vendor specific driver for this device */
        if(usb_bind_vendor_driver(stack, device,
                                  stack->usb_stack.class_driver_list_head, 0))
        {
            /* if so, standard class drivers will be skipped */
            *new_device = device;
            rollback = 1;
        }
    }

    if (!rollback)
    {
        /* find standard class driver for this device */
        status = usb_bind_standard_driver(stack, device, bus);

        if (status != NU_SUCCESS)
        {
#if (USB_TEST_MODE_SUPPORT == NU_TRUE)
            if (device->device_descriptor.idVendor == NU_USB_OTG_TEST_DEV_VID)
            {
                /* Enable Test Mode Based on PID of device*/
                status = usbh_test_mode_enable_using_pid
                                ((NU_USBH_STACK *) stack, device, device->device_descriptor.idProduct);
            }
#else
            /* is this an OTG test device? */
            if (!((device->device_descriptor.idVendor == NU_USB_OTG_TEST_DEV_VID) &&
              (device->device_descriptor.idProduct == NU_USB_OTG_TEST_DEV_PID)) )
            {
                /* if not, this is an unsupported device */
                internal_sts |= NU_USB_STACK_OTG_Report_Status((NU_USB_STACK *)stack,
                                           NU_USB_UNSUPPORTED_DEVICE);
            }

            /* no class driver for this device, device will be suspended */
            internal_sts |= NU_USBH_STACK_Suspend_Device (stack, device);
            rollback =8;
#endif
        }
        else
        {
            *new_device = device;
            rollback = 1;
        }

    } /* if (!rollback) */

    switch (rollback)
    {
        case 8:
            for (i = 0; i < device->device_descriptor.bNumConfigurations; i++)
            {
                if (device->config_descriptors[i])
                {
                    internal_sts |= USB_Deallocate_Memory (device->config_descriptors[i]);
                    device->config_descriptors[i] = NU_NULL;
                }
                if (device->raw_descriptors[i][device->speed])
                {
                    internal_sts |= USB_Deallocate_Memory (device->raw_descriptors[i][device->speed]);
                    device->raw_descriptors[i][device->speed] = NU_NULL;
                }
            }

            for (i = 0; i < NU_USB_MAX_STRINGS; i++)
            {
                if (device->string_descriptors[i])
                {
                    internal_sts |= USB_Deallocate_Memory (device->string_descriptors[i]);
                    device->string_descriptors[i] = NU_NULL;
                }
            }

        case 7:
            internal_sts |= NU_USB_HW_Close_Pipe ((NU_USB_HW *) bus->
                                  controller, device->function_address, 0x80);
        case 6:
            NU_Remove_From_List ((CS_NODE **) & (bus->dev_list),
                                  (CS_NODE *) device);
            if (parent)
                usb_release_id (bus->dev_ids, USB_MAX_DEVID / 8,
                                function_address);
        case 5:
            internal_sts |= NU_Delete_Semaphore(&(device->lock));
        case 4:
            internal_sts |= NU_USB_HW_Close_Pipe ((NU_USB_HW *) bus->
                                  controller, device->function_address, 0x80);
        case 3:
            internal_sts |= USB_Deallocate_Memory (device);
        case 2:
            /* Report status that attached device is not supported. */
            internal_sts |= NU_USB_STACK_OTG_Report_Status((NU_USB_STACK *)stack,
                                            NU_USB_UNSUPPORTED_DEVICE);
        case 1:
            /* De-allocate temporary buffer now if allocated above. */
            if(temp_buffer != NU_NULL)
            {
                internal_sts |= USB_Deallocate_Memory(temp_buffer);
            }

            NU_UNUSED_PARAM(internal_sts);
            return (status);
    }

    return (NU_SUCCESS);
}

#endif   /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */

/*************************************************************************
* FUNCTION
*    USBH_Deenumerate_Device
*
* DESCRIPTION
*     De-enumerates an enumerated device. This API is invoked by Hub class
* driver upon detecting a port disconnect event on its parent port. For
* root hubs, this is called from NU_USB_Remove_HW API. If any driver(s)
* is (are) attached to the device their disconnect callback is called.
* Thereafter, all the resources allocated to the device are released.
*
* INPUTS
*    stack - ptr to stack control block.
*    dev -  ptr to device control block.
* OUTPUTS
*     STATUS  NU_SUCCESS Indicates successful completion of the service
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
STATUS USBH_Deenumerate_Device (NU_USBH_STACK * stack,
                                NU_USB_DEVICE * dev)
{
    USBH_BUS_RESOURCES  *bus;
    UINT8               i;
    STATUS              status;
    STATUS              internal_sts = NU_SUCCESS;
    NU_USB_DRVR         *driver;
    NU_USB_DEVICE       *tmp_usb_device;

    if(stack == NU_NULL
       || dev == NU_NULL)
    {
        return NU_USB_INVLD_ARG;
    }

    /* exclusive access to the device */
    status = NU_USB_DEVICE_Lock(dev);

    if (status != NU_SUCCESS)
    {
        return (status);
    }

    /* check if this is a valid device */
    if (dev->state == USB_STATE_DETACHED)
    {
        internal_sts = NU_USB_DEVICE_Unlock (dev);
        NU_UNUSED_PARAM(internal_sts);

        return NU_USB_DEVICE_DISCONNECTED;
    }
    else
    {
        dev->state = USB_STATE_DETACHED;
    }

    /* find the associated bus */
    bus = usbh_find_bus(stack, dev);

    if(bus == NU_NULL)
    {
        internal_sts = NU_USB_DEVICE_Unlock (dev);
        NU_UNUSED_PARAM(internal_sts);
        return NU_USB_INVLD_ARG;
    }

    /* Invoke driver's disconnect function, if one exists */
    if (dev->driver)
    {
        internal_sts = NU_USB_DRVR_Disconnect (dev->driver,
                                               (NU_USB_STACK *) stack,
                                               (NU_USB_DEVICE *) dev);
    }
    else
    {
        if (dev->active_cnfg_num != NU_USB_MAX_CONFIGURATIONS)
        {
            /* Detachment of device that has an associated driver */
            for (i = 0; i < NU_USB_MAX_INTERFACES; i++)
            {
                driver = dev->config_descriptors[dev->active_cnfg_num]->
                                        intf[i].driver;

                if (driver)
                {
                    internal_sts = NU_USB_DRVR_Disconnect (driver, (NU_USB_STACK *) stack,
                                            (NU_USB_DEVICE *) dev);
                }
            }
        }
    }

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

    if (dev->active_cnfg_num != NU_USB_MAX_CONFIGURATIONS)
    {
	    internal_sts |= usb_close_pipes (dev, bus, NU_TRUE);
    }

#else

    if (dev->active_cnfg_num != NU_USB_MAX_CONFIGURATIONS)
    {
	    internal_sts |= usb_unset_config (stack, dev, bus);
    }

    /* close the default pipe */
    internal_sts |= NU_USB_HW_Close_Pipe((NU_USB_HW *)bus->controller,
                                         dev->function_address, 0x80);

#endif

    /* release all s/w resources of the device */
    for (i = 0; i < dev->device_descriptor.bNumConfigurations; i++)
    {
        if (dev->config_descriptors[i])
        {
            internal_sts |= USB_Deallocate_Memory (dev->config_descriptors[i]);
        }
        if (dev->raw_descriptors[i][dev->speed])
        {
            internal_sts |= USB_Deallocate_Memory (dev->raw_descriptors[i][dev->speed]);
        }
    }

    for (i = 0; i < NU_USB_MAX_STRINGS; i++)
    {
        if (dev->string_descriptors[i])
            internal_sts |= USB_Deallocate_Memory (dev->string_descriptors[i]);
    }

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

    if (dev->bos)
    {
       internal_sts |= USB_Deallocate_Memory (dev->bos);
       dev->bos = NU_NULL;
    }

    if(dev->raw_bos_descriptors)
    {
       internal_sts |= USB_Deallocate_Memory (
                                       dev->raw_bos_descriptors);
       dev->raw_bos_descriptors = NU_NULL;
    }

#endif      /* USB_SPECS_COMPATIBILITY == USB_VERSION_3_0. */

    internal_sts |= NU_Delete_Semaphore(&(dev->lock));

    /* exclusive access to device list */
    status = _NU_USBH_STACK_Lock ((NU_USB_STACK *) stack);

    if (status != NU_SUCCESS)
    {
        NU_UNUSED_PARAM(internal_sts);
        return (status);
    }

    usb_release_id (bus->dev_ids, USB_MAX_DEVID / 8, dev->function_address);

    NU_Remove_From_List ((CS_NODE **) & (bus->dev_list), (CS_NODE *) dev);

    /* If this was the last device then request hardware to go to power down mode. */
    tmp_usb_device = NU_USBH_STACK_Get_Devices((NU_USBH_HW*)dev->hw);

    if(tmp_usb_device != NU_NULL)
    {
        /* Ensure it is root hub and only root hub is present. */
        if ( (tmp_usb_device->function_address == USB_ROOT_HUB) &&
             (tmp_usb_device == (NU_USB_DEVICE*)tmp_usb_device->node.cs_next))
        {
            NU_USB_HW_Request_Power_Down_Mode(dev->hw);
        }
    }

    internal_sts |= USB_Deallocate_Memory (dev);

    internal_sts |= _NU_USBH_STACK_Unlock ((NU_USB_STACK *) stack);

    return internal_sts;
}

/*************************************************************************
*
* FUNCTION
*      USBH_Get_Parent
*
* DESCRIPTION
*    This function gets the pointer to the device control block of the
*    parent device.
*
* INPUTS
*    stack  ptr to stack control block.
*    dev    ptr to device control block, whose parent is to be found.
*    parent On successful return, this would contain the pointer to
*           device control block of the parent device.
*
* OUTPUTS
*   STATUS   NU_SUCCESS  Indicates that the parent is found.
*            NU_INVALID_SEMAPHORE Indicates the semaphore pointer is
*                   invalid.
*            NU_SEMAPHORE_DELETED Semaphore was deleted while the task
*                   was suspended.
*            NU_UNAVAILABLE Indicates the semaphore is unavailable.
*            NU_INVALID_POOL Indicates the dynamic memory pool is
*                   invalid.
*            NU_INVALID_SUSPEND Indicates that this API is called from
*                a non-task thread.
*            NU_USB_INVLD_ARG Indicates that one or more args passed to this
*            API are invalid.
*
*************************************************************************/
STATUS USBH_Get_Parent (NU_USBH_STACK * stack,
                        NU_USB_DEVICE * dev,
                        NU_USB_DEVICE ** parent)
{
    STATUS status, internal_sts = NU_SUCCESS;;

    if(stack == NU_NULL
       || dev == NU_NULL
       || parent == NU_NULL)
    {
        return NU_USB_INVLD_ARG;
    }

    status = NU_USB_DEVICE_Lock(dev);
    if (status != NU_SUCCESS)
        return (status);

    /*  Is the parent a valid device */
    if (!_NU_USBH_STACK_Is_Valid_Device ((NU_USB_STACK *) stack, dev))
    {
        internal_sts = NU_USB_DEVICE_Unlock(dev);
        NU_UNUSED_PARAM(internal_sts);
        return NU_USB_INVLD_ARG;
    }

    /* Root hub has no parent ! */
    if (USBH_IS_ROOT_HUB (dev))
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        *parent = dev->parent;
        status = NU_SUCCESS;
    }

    internal_sts = NU_USB_DEVICE_Unlock(dev);

    return ((status == NU_SUCCESS) ? internal_sts : status);
}

/*************************************************************************
*
* FUNCTION
*      USBH_Get_Port_Number
*
* DESCRIPTION
*    This function finds the port # on the parent hub  to which the
* specified device is connected.
*
* INPUTS
*    stack      ptr to the stack control block.
*    dev        ptr to the device.
*    portNum    On successful return, this would contain the port number.
*
* OUTPUTS
*   STATUS   NU_SUCCESS  Indicates that the parent is found.
*            NU_INVALID_SEMAPHORE Indicates the semaphore pointer is
*                   invalid.
*             NU_SEMAPHORE_DELETED Semaphore was deleted while the task
*                   was suspended.
*             NU_UNAVAILABLE Indicates the semaphore is unavailable.
*             NU_INVALID_POOL Indicates the dynamic memory pool is
*                   invalid.
*             NU_INVALID_SUSPEND Indicates that this API is called from
*                a non-task thread.
*            NU_USB_INVLD_ARG Indicates that one or more args passed to
*            this API are invalid.
*
*************************************************************************/
STATUS USBH_Get_Port_Number (NU_USBH_STACK * stack,
                             NU_USB_DEVICE * dev,
                             UINT8 *port_number)
{
    STATUS status, internal_sts = NU_SUCCESS;

    if(stack == NU_NULL
       || dev == NU_NULL
       || port_number == NU_NULL)
    {
        return NU_USB_INVLD_ARG;
    }

    status = NU_USB_DEVICE_Lock(dev);
    if (status != NU_SUCCESS)
        return (status);

    /* Is this a valid device? */
    if (!_NU_USBH_STACK_Is_Valid_Device ((NU_USB_STACK *) stack, dev))
    {
        internal_sts = NU_USB_DEVICE_Unlock(dev);
        NU_UNUSED_PARAM(internal_sts);
        return NU_USB_INVLD_ARG;
    }

    /* Root hub cannot have a parent ! */
    if (USBH_IS_ROOT_HUB (dev))
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        *port_number = dev->port_number;
        status = NU_SUCCESS;
    }

    internal_sts = NU_USB_DEVICE_Unlock(dev);

    return ((status == NU_SUCCESS) ? internal_sts : status);
}

/*************************************************************************
*
* FUNCTION
*     usbh_hisr
*
* DESCRIPTION
*     This is the HISR that handles all the IRQs of USB host controllers.
*     It releases irq_semaphore thats waited upon by the USBHStk task.
*     The actual processing couldn't be done in the HISR, because
*     NU_SUSPEND option can only be used from a task context.
*
* INPUTS
*     None
*
* OUTPUTS
*     None
*
*************************************************************************/
VOID usbh_hisr (VOID)
{
    NU_USBH_STACK *stack;
    NU_USBH_HISR *stack_hisr;
    STATUS internal_sts = NU_SUCCESS;

    stack_hisr = (NU_USBH_HISR *) NU_Current_HISR_Pointer ();

    if (stack_hisr)
    {
        stack = stack_hisr->stack;

        /* Wake up the USBHStk task */
        internal_sts = NU_Release_Semaphore (&(stack->irq_semaphore));
    }
    NU_UNUSED_PARAM(internal_sts);
}

/*************************************************************************
*
* FUNCTION
*    usbh_scan_controllers
*
* DESCRIPTION
*    This is the entry function of the task USBHStk. It scans all the
*    host controllers and invokes their IRQ handlers,  which go ahead and
*    process pending interrupts, if any.
*
* INPUTS
*    dummy1    for sake of matching the argument prototype of the function
*              pointer passed to NU_Create_Task.
*
*    dummy2    Pointer to the stack control block, that this task belongs.
*
* OUTPUTS
*    None
*
*************************************************************************/
VOID usbh_scan_controllers (UNSIGNED dummy1,
                            VOID *dummy2)
{
    UINT8         i;
    NU_USBH_STACK *stack = (NU_USBH_STACK *) dummy2;
    NU_USBH_HW    *controller;
    STATUS internal_sts = NU_SUCCESS;

    while (NU_Obtain_Semaphore (&(stack->irq_semaphore), NU_SUSPEND) ==
           NU_SUCCESS)
    {
        for (i = 0; i < NU_USBH_MAX_HW; i++)
        {
            controller = stack->bus_resources[i].controller;
            if ((controller) && (controller->pending == NU_TRUE))
            {
                internal_sts = NU_USB_HW_ISR ((NU_USB_HW *) controller);
                controller->pending = NU_FALSE;
                internal_sts |=
                NU_USB_HW_Enable_Interrupts ((NU_USB_HW *)controller);

                NU_UNUSED_PARAM(internal_sts);
            }
        }
    }
}

/*************************************************************************
*
* FUNCTION
*      usb_callback_hndlr
*
* DESCRIPTION
*    This is the callback function used in the Control IRP of the USBD.
* It notifies the completion of the IRP by releasing callback_semaphore.
*
* INPUTS
*    pipe pipe on which the IRP was submitted.
*    irp  ptr to the IRP that has been completed.
* OUTPUTS
*    None
*
*************************************************************************/
VOID usb_callback_hndlr (NU_USB_PIPE * pipe,
                         NU_USB_IRP * irp)
{
    NU_USBH_STACK *stack;
    STATUS irp_status, internal_sts = NU_SUCCESS;

    if(pipe == NU_NULL
       || irp == NU_NULL)
    {
        return ;
    }

    stack = (NU_USBH_STACK *) (pipe->device->stack);

    internal_sts = NU_USB_IRP_Get_Status ((NU_USB_IRP *) irp, &irp_status);

    /* Wake up the task thats waiting after invoking NU_USB_Submit_IRP */
    if(irp_status != NU_USB_IRP_CANCELLED)
    {
        internal_sts |= NU_Release_Semaphore (&(stack->callback_semaphore));
    }
    NU_UNUSED_PARAM(internal_sts);
}


/*************************************************************************
*
* FUNCTION
*      usb_control_transfer
*
* DESCRIPTION
*    Submits a control IRP to of the specified pipe and waits
*    for IRP completion.
*
* INPUTS
*   stack    pointer to stack control block.
*   pipe     pipe over which the IRP has to be transferred.
*   irp      pointer to the control IRP control block.
*   buffer   pointer to location where the data to be sent is
*            found or to location where the data received from
*            this transfer is to be placed.
*   len      number of bytes to be sent or received in this transfer.
*
* OUTPUTS
*   UINT32 Number of data bytes actually sent/received in this transfer.
*          If its an out transfer with no data phase, then length of setup
*          is returned. 0 is returned if the transfer couldn't be completed
*          successfully.
*
*************************************************************************/
UINT32 usb_control_transfer (NU_USBH_STACK * stack,
                             NU_USB_PIPE * pipe,
                             NU_USBH_CTRL_IRP * irp,
                             VOID *buffer,
                             UINT32 len)
{
    UINT32 size = 0;
    STATUS status, irp_status;
    STATUS internal_sts = NU_SUCCESS;
    UINT8 direction;


    if(stack == NU_NULL
       ||pipe == NU_NULL
       || irp == NU_NULL)
    {
        return 0;
    }


    /* Only one control transfer can be outstanding at any time */
    status = NU_Obtain_Semaphore (&(stack->ctrl_msgs_semaphore), NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        return 0;
    }

    /* Place the data in the IRP */
    internal_sts = NU_USB_IRP_Set_Data ((NU_USB_IRP *) irp, buffer);
    internal_sts |= NU_USB_IRP_Set_Length ((NU_USB_IRP *) irp, len);

    /* Control transfer IRP always has a cachable buffer. */
    internal_sts |= NU_USB_IRP_Set_Buffer_Type_Cachable ((NU_USB_IRP *) irp, NU_TRUE);

    /* Send the IRP */
    status = NU_USB_Submit_IRP ((NU_USB_STACK *) stack, (NU_USB_IRP *) irp,
                                pipe);
    if (status != NU_SUCCESS)
    {
        internal_sts |= NU_Release_Semaphore (&(stack->ctrl_msgs_semaphore));
        NU_UNUSED_PARAM(internal_sts);
        return size;
    }
    /* Wait for notification of IRP completion */
    status = NU_Obtain_Semaphore (&(stack->callback_semaphore), (NU_PLUS_Ticks_Per_Second * 5));

    if(status != NU_SUCCESS)
    {
        internal_sts |= NU_USB_PIPE_Flush(pipe);

        /* Reset semaphore counter to 0 */
        internal_sts |= NU_Reset_Semaphore(&(stack->callback_semaphore), 0);

        internal_sts |= NU_Release_Semaphore (&(stack->ctrl_msgs_semaphore));
        NU_UNUSED_PARAM(internal_sts);

        return size;
    }

    status = NU_USB_IRP_Get_Status ((NU_USB_IRP *) irp, &irp_status);

    if ((status != NU_SUCCESS) || (irp_status != NU_SUCCESS))
    {
        internal_sts |= NU_Release_Semaphore (&(stack->ctrl_msgs_semaphore));
        NU_UNUSED_PARAM(internal_sts);

        return size;
    }
    internal_sts |= NU_USB_IRP_Get_Actual_Length ((NU_USB_IRP *) irp, &size);

    /* An out transfer with no data phase, just count bytes in the setup pkt */
    internal_sts |= NU_USBH_CTRL_IRP_Get_Direction (irp, &direction);
    if ((direction == USB_DIR_OUT) && (size == 0))
    {
        size = sizeof (NU_USB_SETUP_PKT);
    }

    internal_sts |= NU_Release_Semaphore (&(stack->ctrl_msgs_semaphore));
    NU_UNUSED_PARAM(internal_sts);

    return size;
}

/*************************************************************************
*
* FUNCTION
*      usb_get_configuration
*
* DESCRIPTION
*     Fetches  all the configuration descriptors and parses each of them,
* provided the device descriptor is found to be a meaningful one.
*
* INPUTS
*   dev   ptr to the device,  whose configuration is
*                          to be fetched.
* OUTPUTS
*    STATUS  NU_SUCCESS  Indicates that all the descriptors have been
*                          successfully fetched from the device & parsed.
*            NU_USB_INVLD_DESC Indicates that one of the descriptors is found
*                               to be invalid.
*            NU_NO_MEMORY Indicates failure of memory allocation
*
*************************************************************************/
STATUS usb_get_configuration (NU_USBH_STACK * cb,
                              NU_USB_DEVICE * dev)
{
    STATUS internal_sts = NU_SUCCESS;
    UINT8 cfgno;
    UINT32 length, len;
    UINT8* temp_buffer = NU_NULL; /* Buffer pointer to fetch the first 8 bytes of the descriptor */
    NU_USB_CFG_DESC *desc;
    NU_USB_PIPE *pipe;
    UINT8 rollback = 0;
    UINT16 ii = 0, jj=0;
    NU_USB_INTF *intf;
    NU_USB_ALT_SETTG *alt_settg;
    UINT8 role, capability;
    STATUS status,mem_status = NU_SUCCESS;
    UINT32 irp_len = 0x00;

    if(cb == NU_NULL
       ||dev == NU_NULL)
    {
        return NU_USB_INVLD_ARG;
    }

    pipe = &dev->ctrl_pipe;

    /* Does it look like a meaningful device descriptor ? */
    if (dev->device_descriptor.bNumConfigurations > NU_USB_MAX_CONFIGURATIONS)
        return (NU_USB_INVLD_DESC);

    if (dev->device_descriptor.bNumConfigurations == 0)
        return (NU_USB_INVLD_DESC);

    /* Allocate uncached memory buffer for data IO. */
    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                 8,
                                 (VOID **) &temp_buffer);
    if(status != NU_SUCCESS)
    {
        return (status);
    }

    /* Seems to be a valid device descriptor, proceed further */
    for (cfgno = 0; cfgno < NU_USB_MAX_CONFIGURATIONS; cfgno++)
    {
        dev->raw_descriptors[cfgno][dev->speed] = NU_NULL;
        dev->config_descriptors[cfgno] = NU_NULL;
    }

    desc = (NU_USB_CFG_DESC *) temp_buffer;

    for (cfgno = 0; cfgno < dev->device_descriptor.bNumConfigurations; cfgno++)
    {
        /* Read the first 8 bytes to know the Config Descriptor's length */
        internal_sts = USBH_FILL_CNTRLPKT ((cb->ctrl_irp),
                            (USB_REQ_RECP_DEV | USB_REQ_STD | USB_REQ_IN),
                            USB_GET_DESCRIPTOR, (USB_DT_CONFIG << 8) + cfgno, 0,
                            8);
        NU_UNUSED_PARAM(internal_sts);

        /* Zero out buffer contents initially. */
        memset(temp_buffer, 0, 8);

        len = usb_control_transfer (cb, pipe, cb->ctrl_irp, temp_buffer, 8);

        if (len < 8)
        {
            internal_sts = NU_USB_INVLD_DESC;
            rollback = 1;
            break;
        }

        /* Convert to CPU's endianness */
        length = LE16_2_HOST (desc->wTotalLength);

        internal_sts = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                           length,
                                           (VOID **) &(dev->raw_descriptors[cfgno])[dev->speed]);

        if (internal_sts != NU_SUCCESS)
        {
            rollback = 1;
            break;
        }

        /* Now get the Complete descriptor */
        internal_sts = USBH_FILL_CNTRLPKT ((cb->ctrl_irp),
                            (USB_REQ_RECP_DEV | USB_REQ_STD | USB_REQ_IN),
                            USB_GET_DESCRIPTOR, (USB_DT_CONFIG << 8) + cfgno, 0,
                            length);
        NU_UNUSED_PARAM(internal_sts);

        if(length % dev->device_descriptor.bMaxPacketSize0)
        {
            irp_len = length;
        }
        else
        {
            irp_len = 0xFF;
        }

        len = usb_control_transfer (cb, pipe, cb->ctrl_irp,
                                    dev->raw_descriptors[cfgno][dev->speed], irp_len);
        if (len < length)
        {
            internal_sts = NU_USB_INVLD_DESC;
            rollback = 1;
            break;
        }

        internal_sts = USB_Allocate_Object(sizeof (NU_USB_CFG),
                                           (VOID **)&(dev->config_descriptors[cfgno]));
        if (internal_sts != NU_SUCCESS)
        {
            rollback = 1;
            break;
        }
        memset ((UINT8 *)dev->config_descriptors[cfgno], 0, sizeof(NU_USB_CFG));

        /* Parse the descriptors */
        internal_sts = USB_Parse_Descriptors ((NU_USB_DEVICE *) dev,
                                   dev->raw_descriptors[cfgno][dev->speed],
                                   dev->config_descriptors[cfgno], (UINT16)length);
        if (internal_sts != NU_SUCCESS)
        {
            rollback = 1;
            break;
        }

        /* OTG stuff */
        if(dev->config_descriptors[cfgno]->otg_desc != NU_NULL)
        {
            /* There is a valid OTG descriptor */
            if(!(dev->otg_status & ~0x03))
            {
                /* OTG is not yet configured on this device */
                if(dev->config_descriptors[cfgno]->otg_desc->bmAttributes & 0x02)
                {
                    /* HNP is supported by the device */
                    status = NU_USB_HW_Get_Role(dev->hw, dev->port_number, &role);

                    if((status == NU_SUCCESS) && ((role & 0x07) == 0x07))
                    {
                        /* Mini-AB port, MiniA plug inserted, in Host role.
                         * Check port capabilities. */
                        status = usbh_hub_get_port_capabilities (&cb->hub_driver,
                                dev->parent, dev->port_number, &capability);

                        if(status == NU_SUCCESS)
                        {
                            /* if the hub port supports HNP and SRP */
                            if(capability & 0x03)
                            {
                                internal_sts =
                                USBH_FILL_CNTRLPKT ((cb->ctrl_irp), 0x0, USB_SET_FEATURE, 0x04, 0, 0);

                                len = usb_control_transfer (cb, pipe, cb->ctrl_irp, NU_NULL, 0);

                                internal_sts |=
                                NU_USB_IRP_Get_Status((NU_USB_IRP *)&cb->ctrl_irp, &status);

                                NU_UNUSED_PARAM(len);
                                NU_UNUSED_PARAM(internal_sts);

                                if(status == NU_SUCCESS)
                                {
                                    dev->otg_status |= (0x05 << 4);
                                }
                                /* we enable the HNP on the device when there is a suspend
                                   on the bus.
                                */
                            }
                        }
                    }
                }
            }
        }

        /* Obtain all the string descriptors */
        if(dev->config_descriptors[cfgno]->desc->iConfiguration)
        {
            internal_sts = usb_fetch_string_desc (cb, dev,
                            dev->config_descriptors[cfgno]->desc->iConfiguration);

            if(internal_sts != NU_SUCCESS)
            {
                continue;
            }
        }

        /* for each of the interfaces */
        for(ii=0; ii<dev->config_descriptors[cfgno]->desc->bNumInterfaces; ii++)
        {
            intf = &dev->config_descriptors[cfgno]->intf[ii];

            jj = 0;

            alt_settg = &intf->alt_settg[0];

            internal_sts = NU_SUCCESS;

            while(alt_settg->desc != NU_NULL)
            {
                if (alt_settg->desc->iInterface)
                {
                    internal_sts = usb_fetch_string_desc (cb, dev,
                                       alt_settg->desc->iInterface);

                    if(internal_sts != NU_SUCCESS)
                        break;
                }

                jj++;

                if(jj >= NU_USBH_MAX_ALT_SETTINGS)
                {
                    break;
                }
                alt_settg = &intf->alt_settg[jj];
            }

            if(internal_sts != NU_SUCCESS)
                break;
        }

        internal_sts = NU_SUCCESS;
    }

    internal_sts |= USB_Deallocate_Memory(temp_buffer);

    if (!rollback)
        return (NU_SUCCESS);

    for (cfgno = 0; cfgno < dev->device_descriptor.bNumConfigurations; cfgno++)
    {
        if (dev->config_descriptors[cfgno])
        {
            mem_status = USB_Deallocate_Memory (dev->config_descriptors[cfgno]);
            dev->config_descriptors[cfgno] = NU_NULL;
        }
        if (dev->raw_descriptors[cfgno][dev->speed])
        {
            mem_status |= USB_Deallocate_Memory (dev->raw_descriptors[cfgno][dev->speed]);
            dev->raw_descriptors[cfgno][dev->speed] = NU_NULL;
        }
        NU_UNUSED_PARAM(mem_status);
    }
    return internal_sts;
}

/*************************************************************************
*
* FUNCTION
*      power
*
* DESCRIPTION
*     calculates a raised to b.
* INPUTS
*    UINT8 a, b
* OUTPUTS
*    UINT32 the value of a raised to b.
*
*************************************************************************/
UINT32 power (UINT8 a,
              UINT8 b)
{
    UINT32 i, ret = 1L;

    for (i = 0; i < b; i++)
        ret *= (UINT32) a;

    return ret;
}

/*************************************************************************
*
* FUNCTION
*      usb_interval_2_usecs
*
* DESCRIPTION
*    Translates bInterval value of endpoint descriptor into usecs
*    based on its speed and attributes
*
* INPUTS
*    UINT8 interval   bInterval field of Endpoint descriptor
*    UINT8 attributes bmAttributes field of Endpoint descriptor
*    UINT8 speed      Speed of the device: USB_SPEED_LOW/FULL/HIGH
*
* OUTPUTS
*     UINT32  Interval value in microseconds.
*
*************************************************************************/
UINT32 usb_interval_2_usecs (UINT8 interval,
                             UINT8 attributes,
                             UINT8 speed)
{
    UINT32 ret = 0;

    switch (attributes & 0x03)
    {
        case 0x00:  /* control endpoint */
        case 0x02:  /* bulk endpoint */
            /* interval specifies maximum NAK rate, 0 = never NAKs
             * interval in range 0...255
             */
            ret = interval;
            break;

        case 0x01:  /* isochronous endpoint */
            /* interval is exponent for value 2**(interval-1)
             * interval must be in range 1...16
             */
            if(interval >= 1 && interval <= 16)
            {
                ret = power (2, interval - 1);
            }
            break;

        case 0x03:  /* interrupt endpoint */
            if (speed == USB_SPEED_HIGH)
            {
                /* high-speed interrupt endpoint
                 * interval is exponent for value 2**(interval-1)
                 * interval must be in range 1...16
                 */
                if(interval >= 1 && interval <= 16)
                {
                    ret = power (2, interval - 1);
                }
            }
            else
            {
                /* full/low-speed interrupt endpoint
                 * interval must be in range 1...255
                 */
                if(interval >= 1)
                {
                    ret = interval;
                }
            }
            break;
    }

    /* interval is in frames or microframes depending on speed */
    if (speed == USB_SPEED_HIGH)
    {
        /* high-speed: interval in 125us microframes */
        return ret * 125L;
    }
    else
    {
        /* full/low-speed: interval in 1000us frames */
        return ret * 1000L;
    }
}

/*************************************************************************
*
* FUNCTION
*      usb_close_pipes
*
* DESCRIPTION
*     Releases the b/w associated with all the open pipes of the device and
* configures the HC's H/W to release the resources of these open pipes. IRPS
* if any are pending on these pipes, this would result in invocation of their
* callbacks with irp->status set to NU_USB_IRP_CANCELLED.
*
* INPUTS
*    dev Ptr to device structure whose pipes are to be closed.
*    bus Ptr to the bus structure to which the device belongs.
*
* OUTPUTS
*     STATUS   NU_SUCCESS Indicates successful completion of closing pipes.
*              NU_USB_DEV_NOT_CONFIGURED Indicates that the device doesn't
*                  have any active configuration set.
*
*************************************************************************/
STATUS usb_close_pipes (NU_USB_DEVICE * dev,
                        USBH_BUS_RESOURCES * bus, UINT8 close_default_pipe)
{
    STATUS internal_sts = NU_SUCCESS;
    NU_USB_CFG *cnfg;
    UINT8 interface_no, i;
    NU_USB_ALT_SETTG *alt_set;
    UINT8 bEndpointAddress = 0x80;          /* The endpoint 0 */

    if(dev == NU_NULL
       ||bus == NU_NULL)
    {
        return NU_USB_INVLD_ARG;
    }

    if (dev->active_cnfg_num == NU_USB_MAX_CONFIGURATIONS)
        return NU_USB_DEV_NOT_CONFIGURED;

    cnfg = dev->config_descriptors[dev->active_cnfg_num];

    if (cnfg == NU_NULL)
        return NU_USB_INVLD_ARG;

    if(close_default_pipe == NU_TRUE)
    {
        /* Close the default pipe */
        internal_sts = NU_USB_HW_Close_Pipe ((NU_USB_HW *) bus->controller,
                                             dev->function_address,
                                             bEndpointAddress);
    }

    /* To find the open  pipes, walk through the endpoint descriptors on
     * the active alternate setting of each interface of the active configuration
     */
    for (interface_no = 0;
         interface_no < cnfg->desc->bNumInterfaces; interface_no++)
    {
        /* The active alternate setting */
        alt_set = dev->config_descriptors[dev->active_cnfg_num]->
            intf[interface_no].current;
        if (alt_set)
        {
            for (i = 0; i < NU_USB_MAX_ENDPOINTS; i++)
            {
                if (alt_set->endp[i].desc == NU_NULL)
                    continue;
                /* Valid Endpoint descriptor */

                /* invoke Controller's API */
                internal_sts = NU_USB_HW_Close_Pipe ((NU_USB_HW *) bus->controller,
                                                     dev->function_address,
                                                     alt_set->endp[i].desc->bEndpointAddress);
            }
            dev->config_descriptors[dev->active_cnfg_num]->
                intf[interface_no].current = NU_NULL;
        }
    }

    /* Release the bus bandwidth of this configuration */
    bus->avail_bandwidth += cnfg->load;
    dev->active_cnfg_num = NU_USBH_MAX_CONFIGURATIONS;

    NU_UNUSED_PARAM(internal_sts);
    return (NU_SUCCESS);
}

/*************************************************************************
*
* FUNCTION
*     usb_set_interface_id
*
* DESCRIPTION
*   Sends a SET_FEATURE request to set the alternate setting on the specified
* interface of the device. And also configures the HC's H/W to reflect the
* specified alternate setting.
*
* INPUTS
*    dev Ptr to device structure whose pipes are to be closed.
*    bus Ptr to the bus structure to which the device belongs.
*    cnfg Ptr to the structure holding the parsed descriptors.
*    interface_index bInterfaceNumber on the specified configuration.
*    alt_setting_index bAlternateSetting of the specified interface.
*
* OUTPUTS
*    STATUS
*    NU_SUCCESS            Indicates that the service is successfully completed.
*    NU_INVALID_SEMAPHORE  Indicates the semaphore pointer is invalid.
*    NU_SEMAPHORE_DELETED  Semaphore was deleted while the task was suspended.
*    NU_UNAVAILABLE        Indicates the semaphore is unavailable.
*    NU_USB_SCHEDULE_ERROR Indicate that a periodic pipe of the configuration
*                          couldn't be correctly scheduled by the HCD.
*    NU_NO_MEMORY          Indicates failure of memory allocation
*    NU_USB_INVLD_ARG      Indicates that invalid configuration was passed or
*                          specified index of interface is out of bounds
*
*************************************************************************/
STATUS usb_set_interface_id (NU_USBH_STACK * cb,
                             NU_USB_DEVICE * dev,
                             USBH_BUS_RESOURCES * bus,
                             NU_USB_CFG * cnfg,
                             UINT8 interface_index,
                             UINT8 alt_setting_index,
                             BOOLEAN send_request)
{
    STATUS            status = NU_SUCCESS;
    UINT32            len = 0;
    NU_USB_ALT_SETTG  *alt_set;

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_FALSE )
    STATUS            internal_sts = NU_SUCCESS;
    NU_USB_ENDP_DESC  *ep_cur, *ep_new;
    UINT16            max_pkt_size;
    UINT8             i, j;
    static BOOLEAN    cur_open_pipes[NU_USB_MAX_ENDPOINTS];
#endif

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
    NU_USB_ALT_SETTG  *curr_alt_set;
#endif

    if(cb == NU_NULL
       ||dev == NU_NULL
       || bus == NU_NULL
       || cnfg == NU_NULL
       || (interface_index >= NU_USB_MAX_INTERFACES))
    {
        return NU_USB_INVLD_ARG;
    }

    alt_set = &(cnfg->intf[interface_index].alt_settg[alt_setting_index]);

    /* If valid alternate setting */
    if (alt_set->desc == NU_NULL)
    {
        return NU_USB_INVLD_ARG;
    }

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

    /* for non default alternate setting, the request should be sent */
    if(send_request)
    {
        curr_alt_set = cnfg->intf[interface_index].current;
        /* As per USB 3.0 requirements we first need to check the BW available
         * in the HW before sending request to the device.
         */
        if (curr_alt_set != alt_set)
        {
            status = NU_USBH_HW_Allocate_Bandwidth(cb, dev, NU_NULL, curr_alt_set, alt_set);
        }

        if ( status == NU_SUCCESS)
        {
            /* Send SET_FEATURE to the device */
            status = USBH_FILL_CNTRLPKT ((cb->ctrl_irp),
                                (USB_REQ_RECP_INTF | USB_REQ_STD | USB_REQ_OUT),
                                USB_SET_INTERFACE, alt_setting_index,
                                interface_index, 0);

            /* STALL may be returned if only the default setting is supported
             * therefore, return value of this transfer is ignored
             */
            len = usb_control_transfer (cb, &dev->ctrl_pipe, cb->ctrl_irp, NU_NULL, 0);
            NU_UNUSED_PARAM(len);
        }

        return ( status );
    }

#else

    /* for non default alternate setting, the request should be sent */
    if(send_request)
    {
        /* Send SET_FEATURE to the device */
        internal_sts = USBH_FILL_CNTRLPKT ((cb->ctrl_irp),
                            (USB_REQ_RECP_INTF | USB_REQ_STD | USB_REQ_OUT),
                            USB_SET_INTERFACE, alt_setting_index,
                            interface_index, 0);

        /* STALL may be returned if only the default setting is supported
         * therefore, return value of this transfer is ignored
         */
        len = usb_control_transfer (cb, &dev->ctrl_pipe, cb->ctrl_irp, NU_NULL, 0);
        NU_UNUSED_PARAM(len);
    }

    /* Setting at the device is successful, now modify it at the HCD */
    if (alt_set != cnfg->intf[interface_index].current)
    {
        memset(cur_open_pipes, NU_FALSE, sizeof(BOOLEAN)*NU_USB_MAX_ENDPOINTS);

        for (i = 0; i < NU_USB_MAX_ENDPOINTS; i++)
        {
            ep_new = alt_set->endp[i].desc;
            if (ep_new == NU_NULL)
                continue;

            max_pkt_size = ep_new->wMaxPacketSize0;
            max_pkt_size |= (ep_new->wMaxPacketSize1 << 8);

            if (cnfg->intf[interface_index].current)
            {
                /* Interface already has an active alternate
                 * setting with opened pipes!
                 */
                for (j = 0; j < NU_USB_MAX_ENDPOINTS; j++)
                {
                    ep_cur = cnfg->intf[interface_index].current->endp[j].desc;

                    if ((ep_cur) && (ep_cur->bEndpointAddress ==
                                    ep_new->bEndpointAddress))
                    {
                        if ((ep_cur->wMaxPacketSize0 != ep_new->wMaxPacketSize0)
                         || (ep_cur->wMaxPacketSize1 != ep_new->wMaxPacketSize1)
                         || (ep_cur->bInterval != ep_new->bInterval)
                         || (ep_cur->bmAttributes != ep_new->bmAttributes))
                        {
                            /* Change in pipe attributes, inform HCD */
                            status = NU_USB_HW_Modify_Pipe
                                ((NU_USB_HW *)bus->controller,
                                 dev->function_address,
                                 ep_cur->bEndpointAddress,
                                 ep_new->bmAttributes,
                                 max_pkt_size,
                                 usb_interval_2_usecs
                                   (ep_new->bInterval, ep_new->bmAttributes, dev->speed),
                                 alt_set->endp[i].load);

                            if (status != NU_SUCCESS)
                                return (status);
                        }

                        cur_open_pipes[j] = NU_TRUE;
                        break;
                    }
                }
            }
            else
            {
                j = NU_USB_MAX_ENDPOINTS;
            }

            if (j == NU_USB_MAX_ENDPOINTS)
            {
                /* The pipe doesn't exist in the H/W, create it */
                status = NU_USB_HW_Open_Pipe((NU_USB_HW *) bus->controller,
                            dev->function_address,
                            ep_new->bEndpointAddress,
                            ep_new->bmAttributes,
                            dev->speed,
                            max_pkt_size,
                            usb_interval_2_usecs
                             (ep_new->bInterval, ep_new->bmAttributes, dev->speed),
                            alt_set->endp[i].load);

                if (status != NU_SUCCESS)
                {
                    /* Report status that problem occurred in opening the pipe */
                    internal_sts |= NU_USB_STACK_OTG_Report_Status((NU_USB_STACK *)cb, status);
                    NU_UNUSED_PARAM(internal_sts);

                    return (status);
                }
            }
        }

        if (cnfg->intf[interface_index].current != NU_NULL)
        {
            for (i = 0; i < NU_USB_MAX_ENDPOINTS; i++)
            {
                ep_cur = cnfg->intf[interface_index].current->endp[i].desc;
                if (ep_cur == NU_NULL)
                    continue;

                /*The pipe is either required as open, or has been modified */
                if (cur_open_pipes[i] == NU_TRUE)
                    continue;

                status = NU_USB_HW_Close_Pipe((NU_USB_HW *) bus->controller,
                                        dev->function_address,
                                        ep_cur->bEndpointAddress);

                if (status != NU_SUCCESS)
                {
                    /* Report status that problem occurred in closing the pipe */
                    internal_sts |= NU_USB_STACK_OTG_Report_Status((NU_USB_STACK *)cb, status);
                    NU_UNUSED_PARAM(internal_sts);

                    return (status);
                }
            }
        }
    }

#endif

    /* Mark the previous alt setting as inactive */
    if (cnfg->intf[interface_index].current)
        cnfg->intf[interface_index].current->is_active = 0;

    /* Mark the new one as the active alternate setting */
    cnfg->intf[interface_index].current = alt_set;
    cnfg->intf[interface_index].current->is_active = 1;

    return (NU_SUCCESS);
}

/*************************************************************************
*
* FUNCTION
*      usb_find_next_best_vendor_drvr
*
* DESCRIPTION
*   Finds a best matching USB Device driver that matches the vendor id/product
*   id/device release number. The match flag determines which of these fields
*   need to be considered.
*
* INPUTS
*  cb ptr to stack control block.
*  device_descriptor ptr to descriptor that has to be examined for a match.
*  current The marker into the class driver db, at which matching has to begin.
*  only_this_driver If 1 , this means that the matching has to be performed
*  only on the driver specified by current and so shouldn't search the rest
*  of the driver database.
*
* OUTPUTS
*    NU_USB_DRIVER *    Ptr to matching USB Device driver or NU_NULL, if no match
*                         could be found.
*
*************************************************************************/
NU_USB_DRVR *usb_find_next_best_vendor_drvr (NU_USBH_STACK * cb,
                                             NU_USB_DEVICE_DESC *device_descriptor,
                                             USB_DRVR_LIST * current,
                                             UINT8 only_this_driver)
{
    /* Caller of this holds driver database lock */
    NU_USB_DRVR *next;
    STATUS status;
    USB_DRVR_LIST *next_node;

    if(cb == NU_NULL
       ||device_descriptor == NU_NULL
       || current == NU_NULL)
    {
        return NU_NULL;
    }

    next = current->driver;
    next_node = current;

    /* Driver database is maintained in the descending order of
     * score, so the matches are automatically the best ones.
     */
    while (next)
    {
        status = NU_USB_DRVR_Examine_Device (next, device_descriptor);
        if (status == NU_SUCCESS)
        {
            return next;
        }

        next_node = (USB_DRVR_LIST *) (next_node->list_node.cs_next);
        next = next_node->driver;

        /* End of the class drivers list ?? */
        if ((next == cb->usb_stack.class_driver_list_head->driver)
            || (only_this_driver))
            next = NU_NULL;
    }
    return NU_NULL;
}

/*************************************************************************
*
* FUNCTION
*      usb_bind_vendor_driver
*
* DESCRIPTION
*   Check vendor specific drivers for the given device.
*   If found, initialization routine of the vendor driver will be called.
*
* INPUTS
*   stack   pointer to stack control block.
*   device  pointer to the device to be initialized
*   driver  header of the driver list to be searched
*   only_this_driver  flag to determine whether entire driver list will be
*                     searched or just the one that pointed by 'driver'
*
* OUTPUTS
*    NU_USB_DRIVER *  pointer to the vendor driver that found and initialized
*
*************************************************************************/
NU_USB_DRVR *usb_bind_vendor_driver (NU_USBH_STACK *stack,
                                     NU_USB_DEVICE *device,
                                     USB_DRVR_LIST *driver,
                                     UINT8 only_this_driver)
{
    NU_USB_DRVR *vendor;
    STATUS status;

    if(stack == NU_NULL
       ||device == NU_NULL
       || driver == NU_NULL)
    {
        return NU_NULL;
    }

    /* see if there is vendor specific driver for this device */
    vendor = usb_find_next_best_vendor_drvr(stack, &(device->device_descriptor),
                          driver, only_this_driver);

    /* if there is, call its initialization routine */
    if(vendor)
    {
        status = NU_USB_DRVR_Initialize_Device(vendor, (NU_USB_STACK *)stack, device);
        if((status == NU_SUCCESS) && (device->driver))
        {
            return vendor;
        }
    }

    return NU_NULL;
}

/*************************************************************************
*
* FUNCTION
*      usb_find_next_best_std_driver
*
* DESCRIPTION
*   Finds a best matching USB interface driver that matches the class/sub-class
*   /protocol.
*
* INPUTS
*
*  cb ptr to stack control block.
*  device_descriptor ptr to descriptor that has to be examined for a match.
*  current The marker into the class driver db, at which matching has to begin.
*  only_this_driver If 1 , this means that the matching has to be performed
*  only on the driver specified by current and so shouldn't search the rest
*  of the driver database.
*
* OUTPUTS
*    NU_USB_DRIVER *    Ptr to matching USB Device driver or NU_NULL, if no match
*                         could be found.
*
*************************************************************************/
USB_DRVR_LIST *usb_find_next_best_std_driver (NU_USBH_STACK * cb,
                                            NU_USB_INTF_DESC *interface_descriptor,
                                            USB_DRVR_LIST * current,
                                            UINT8 only_this_driver)
{
    NU_USB_DRVR *next;
    STATUS status;
    USB_DRVR_LIST *next_node;

    if(cb == NU_NULL
       ||interface_descriptor == NU_NULL
       || current == NU_NULL)
    {
        return NU_NULL;
    }

    next = current->driver;
    next_node = current;

    /* Driver database is maintained in the descending order of
     * score, so the matches are automatically the best ones.
     */
    while (next)
    {
        status = NU_USB_DRVR_Examine_Intf (next, interface_descriptor);
        if (status == NU_SUCCESS)
        {
            return next_node;
        }

        next_node = (USB_DRVR_LIST *) (next_node->list_node.cs_next);
        next = next_node->driver;

        /* End of the class drivers list ?? */
        if ((next == cb->usb_stack.class_driver_list_head->driver)
            || (only_this_driver))
            next = NU_NULL;
    }

    return NU_NULL;
}

/*************************************************************************
*
* FUNCTION
*      usb_bind_standard_driver
*
* DESCRIPTION
*   Find standard class driver for the given device.
*   If found, configure the device and call class driver init routine for
*   each interface it has.
*
* INPUTS
*   stack   pointer to stack control block.
*   device  pointer to the device to be initialized
*   bus     pointer to the bus that the device is attached
*
* OUTPUTS
*   STATUS  NU_SUCCESS      when driver successfully initializes the device
*           NU_UNAVAILABLE  driver is not available or failed to initialize
*
*************************************************************************/
STATUS usb_bind_standard_driver(NU_USBH_STACK *stack,
                                NU_USB_DEVICE *device,
                                USBH_BUS_RESOURCES *bus)
{
    INT c, i, a;  /* configuration, interface, alternate setting index */
    INT configured = NU_FALSE;
    INT initialized;
    STATUS status;
    STATUS internal_sts = NU_SUCCESS;
    NU_USB_CFG        *config;
    NU_USB_ALT_SETTG  *alt;
    NU_USB_DRVR       *driver;
    USB_DRVR_LIST     *head, *list;

    if(stack == NU_NULL
       ||device == NU_NULL
       || bus == NU_NULL)
    {
        return NU_USB_INVLD_ARG;
    }

    c = 0;   /* first configuration */

    while ((c < device->device_descriptor.bNumConfigurations) && (!configured))
    {
        /* get configuration descriptor */
        config = device->config_descriptors[c];
        initialized = 0;

        /* for each interface */
        for (i = 0; i < config->desc->bNumInterfaces; i++)
        {
            head = stack->usb_stack.class_driver_list_head;
            driver = NU_NULL;
            list = NU_NULL;

            while(head)
            {
                /* check each alternate setting for standard class driver */
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
                        device->active_cnfg_num = (UINT8)c;
                        configured = NU_TRUE;
                    }
                    else
                    {
                        break;
                    }
                }

                /* device is just configured or has multiple interfaces */
                if((driver) && (configured == NU_TRUE))
                {
                    /* call class driver for the interface */
                    status = NU_USB_DRVR_Initialize_Interface(driver, (NU_USB_STACK *)stack,
                                       device, &config->intf[i]);
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
                            head = NU_NULL;

                    }
                }

                if(driver == NU_NULL)
                    break;

            } /* while(list) */
        } /* for (i = 0; .. */

        /* Did drivers fail to initialize the device ?*/
        if ((configured == NU_TRUE) && (initialized == 0))
        {
            /* move the device back to addressed mode */
            internal_sts = usb_unset_config(stack, device, bus);
            configured = NU_FALSE;
        }

        c++; /* try next configuration */
    } /* while(c < ...) */

    NU_UNUSED_PARAM(internal_sts);
    if(configured == NU_TRUE)
        return (NU_SUCCESS);
    else
        return (NU_UNAVAILABLE);
}

/*************************************************************************
*
* FUNCTION
*      usb_init_interfaces
*
* DESCRIPTION
*   Invoke class driver initialize routine for each interface in the active
*   configuration
*
* INPUTS
*   stack   pointer to stack control block.
*   device  pointer to the device to be initialized
*   list    pointer to the class driver list
*
* OUTPUTS   none
*
*************************************************************************/
VOID usb_init_interfaces(NU_USBH_STACK *stack,
                         NU_USB_DEVICE *device,
                         USB_DRVR_LIST *list)
{
    INT               i, a;    /* interface, alternate setting index */
    NU_USB_CFG        *config;
    NU_USB_ALT_SETTG  *alt;
    USB_DRVR_LIST     *node;
    STATUS status = NU_SUCCESS;

    if(stack == NU_NULL
       ||device == NU_NULL
       || list == NU_NULL)
    {
        return ;
    }
    NU_UNUSED_PARAM(status);

    /* get descriptor of the active configuration */
    config = device->config_descriptors[device->active_cnfg_num];

    if(config == NU_NULL)
        return;

    for (i = 0; i < config->desc->bNumInterfaces; i++)
    {
        /* check each alternate setting for standard class driver */
        for(a = 0; a < NU_USB_MAX_ALT_SETTINGS; a++)
        {
            alt = &(config->intf[i].alt_settg[a]);
            if(alt->desc)
            {
                node = usb_find_next_best_std_driver(stack, alt->desc, list, 1);

                if(node)
                {
                    status = NU_USB_DRVR_Initialize_Interface(node->driver,
                               (NU_USB_STACK *)stack, device, &config->intf[i]);

                    break;
                }
             }
        }
    } /* for (i = 0; .. */

    NU_UNUSED_PARAM(status);
}

/*************************************************************************
*
* FUNCTION
*      usb_attempt_unclaimed_devs
*
* DESCRIPTION
*     This function walks through the device database to find out if there are any
*  enumerated USB devices that don't have any drivers attached. Some of the
*  interfaces on a composite USb device may not have an active driver while
*  some might have already have an active driver. This function attempts to
*  match all such un-attached interfaces of a composite device too.
*
* INPUTS
*    stack ptr to stack control block.
*    drvr identifies the new driver for which unclaimed interfaces/devices are being
*         found.
* OUTPUTS
*    VOID
*
*************************************************************************/
VOID usb_attempt_unclaimed_devs (NU_USBH_STACK * stack,
                                 USB_DRVR_LIST * drvr)
{
    USBH_BUS_RESOURCES *bus;
    NU_USB_DEVICE      *dev;
    NU_USB_CFG         *config;
    UINT8              deviceNeedsDriver, i, j, configured;
    STATUS internal_sts = NU_SUCCESS;

    if(stack == NU_NULL
       ||drvr == NU_NULL)
    {
        return ;
    }

    for (i = 0; i < NU_USBH_MAX_HW; i++)
    {
        bus = &(stack->bus_resources[i]);
        if (!bus->controller)
            continue;
        dev = bus->dev_list;

        while (dev) /* for each device on the bus */
        {
            deviceNeedsDriver = 0;
            configured = 0;

            /* is this device already configured ? */
            if (dev->active_cnfg_num == NU_USB_MAX_CONFIGURATIONS)
            {
                deviceNeedsDriver = 1; /* not configured */
            }
            else
            {
                configured = 1;      /* already configured */

                /* get configuration descriptor */
                config = dev->config_descriptors[dev->active_cnfg_num];

                /* See if any of its interfaces need a driver */
                for (j = 0;j < config->desc->bNumInterfaces; j++)
                {
                    if (config->intf[j].driver == NU_NULL)
                    {
                        deviceNeedsDriver = 1;
                        break;
                    }
                }
            }

            if (deviceNeedsDriver)
            {
                if(configured)
                {
                    /* check interfaces in the chosen configuration */
                    usb_init_interfaces(stack, dev, drvr);
                }
                else
                {
                    /* is there a vendor driver that can drive this? */
                    if (usb_bind_vendor_driver(stack, dev, drvr, 1) == NU_NULL)
                    {
                        /* if not, try standard class drivers */
                        internal_sts = usb_bind_standard_driver(stack, dev, bus);
                    }
                }
            }

            dev = (NU_USB_DEVICE *) dev->node.cs_next;  /* try next device */
            if (dev == bus->dev_list)
                dev = NU_NULL;
        }
    }
    NU_UNUSED_PARAM(internal_sts);
}

/*************************************************************************
*
* FUNCTION
*      usb_any_claimed_device
*
* DESCRIPTION
*    This function walks through the USB device database, to see if there any
* devices that are currently using the specified driver. This function is
* called by NU_USB_Deregister_Driver to ensure that only an inactive driver
* is allowed to deregister.
*
* INPUTS
*    drvr  ptr that identifies the driver whose owned interfaces/devices
*          have to be found.
* OUTPUTS
*   UINT8  1 Indicates that the driver is active and 0 Indicates that it isn't
*
*************************************************************************/
UINT8 usb_any_claimed_device (NU_USBH_STACK * stack,
                              NU_USB_DRVR * drvr)
{
    USBH_BUS_RESOURCES *bus;
    NU_USB_DEVICE *dev;
    UINT8 interface_no;
    UINT8 i;

    if(stack == NU_NULL
       ||drvr == NU_NULL)
    {
        return 0;
    }

    /* For all controllers on the system */
    for (i = 0; i < NU_USBH_MAX_HW; i++)
    {
        bus = &(stack->bus_resources[i]);
        if (!bus->controller)
            continue;
        dev = bus->dev_list;

        /* For every device on the bus */
        while (dev)
        {
            if (dev->active_cnfg_num == NU_USB_MAX_CONFIGURATIONS)
                continue;

            /* Configured device */
            if (dev->driver == drvr)
            {
                return 1;
            }
            /* For every interface on the configured device */
            for (interface_no = 0;
                 interface_no < NU_USB_MAX_INTERFACES; interface_no++)
            {
                /* Has an active alternate setting ? */
                if ((dev->
                     config_descriptors[dev->active_cnfg_num]->
                     intf[interface_no].current)
                    && (dev->
                        config_descriptors[dev->active_cnfg_num]->
                        intf[interface_no].driver == drvr))
                    return 1;
            }
            dev = (NU_USB_DEVICE *) dev->node.cs_next;
            if (dev == bus->dev_list)
                dev = NU_NULL;
        }
    }
    return 0;
}

/*************************************************************************
*
* FUNCTION
*      usb_calc_load
*
* DESCRIPTION
*     This function calculate the bandwidth an endpoint would consume
* in us.  These calculations are lifted from section 5.11.3 of USB
* Specification 2.0.
* INPUTS
*   direction_n_type Transfer type & Transfer direction of the endpoint.
*                          should be one of the PIPE_CONTROL, OUT_PIPE_BULK ,
*                          OUT_PIPE_INTERRUPT, IN_PIPE_BULK, IN_PIPE_INTERRUPT,
*                          IN_PIPE_ISOCHRONOUS, OUT_PIPE_ISOCHRONOUS.
*
*   speed            Speed of the device. Should be one of the
*                          USB_SPEED_HIGH, USB_SPEED_FULL, USB_SPEED_LOW
*   max_packet_size Maximum Pkt size of the endpoint. Cannot be more
*                          than 4096 as per USB2.0.
*
* OUTPUTS
*
*************************************************************************/
UINT32 usb_calc_load (UINT8 direction_n_type,
                      UINT8 speed,
                      UINT16 max_packet_size)
{
    UINT32 x1, x2;

    switch (speed)
    {
        case USB_SPEED_HIGH:
            if ((direction_n_type == PIPE_CONTROL) ||
                (direction_n_type == OUT_PIPE_BULK) ||
                (direction_n_type == OUT_PIPE_INTERRUPT) ||
                (direction_n_type == IN_PIPE_BULK) ||
                (direction_n_type == IN_PIPE_INTERRUPT))
            {
                /* In or Out Non Iso */
                x1 = ((55 * 8 * 2083) / 1000UL);
                x2 = (3167 + (((7 * 8 * max_packet_size) / 6) * 1000UL));
                x2 /= 1000UL;
                x2 = 2083UL * x2;
                x2 /= 1000UL;
                return (x1 + x2 + NU_USB_HOST_DELAY + 500UL) / 1000UL;

            }
            if ((direction_n_type == IN_PIPE_ISOCHRONOUS) ||
                (direction_n_type == OUT_PIPE_ISOCHRONOUS))
            {
                /* In or Out  Iso */
                x1 = ((38 * 8 * 2083) / 1000UL);
                x2 = (3167 + (((7 * 8 * max_packet_size) / 6) * 1000UL));
                x2 /= 1000UL;
                x2 = 2083UL * x2;
                x2 /= 1000UL;
                return (x1 + x2 + NU_USB_HOST_DELAY + 500UL) / 1000UL;

            }
        case USB_SPEED_FULL:
            if ((direction_n_type == PIPE_CONTROL) ||
                (direction_n_type == OUT_PIPE_BULK) ||
                (direction_n_type == OUT_PIPE_INTERRUPT) ||
                (direction_n_type == IN_PIPE_BULK) ||
                (direction_n_type == IN_PIPE_INTERRUPT))
            {
                /* In or Out Non Iso */
                x2 = (3167 + (((7 * 8 * max_packet_size) / 6) * 1000UL));
                x2 /= 1000UL;
                x2 = 8354UL * x2;
                x2 /= 100UL;
                return (9107UL + x2 + NU_USB_HOST_DELAY + 500UL) / 1000UL;

            }
            if (direction_n_type == IN_PIPE_ISOCHRONOUS)
            {
                /* In Iso */
                x2 = (3167 + (((7 * 8 * max_packet_size) / 6) * 1000UL));
                x2 /= 1000UL;
                x2 = 8354UL * x2;
                x2 /= 100UL;
                return (7268UL + x2 + NU_USB_HOST_DELAY + 500UL) / 1000UL;

            }
            if (direction_n_type == OUT_PIPE_ISOCHRONOUS)
            {
                /* Out Iso */
                x2 = (3167 + (((7 * 8 * max_packet_size) / 6) * 1000UL));
                x2 /= 1000UL;
                x2 = 8354UL * x2;
                x2 /= 100UL;
                return (6265UL + x2 + NU_USB_HOST_DELAY + 500UL) / 1000UL;

            }
        case USB_SPEED_LOW:
            if ((direction_n_type == PIPE_CONTROL) ||
                (direction_n_type == OUT_PIPE_BULK) ||
                (direction_n_type == OUT_PIPE_INTERRUPT) ||
                (direction_n_type == OUT_PIPE_ISOCHRONOUS))
            {
                /* Out  Iso and Non Iso */
                x2 = (3167 + (((7 * 8 * max_packet_size) / 6) * 1000UL));
                x2 /= 1000UL;
                x2 = 667UL * x2;
                return (64107UL + (2 * NU_USB_HUB_LS_SETUP_DELAY) + x2 +
                        NU_USB_HOST_DELAY + 500UL) / 1000UL;

            }
            if ((direction_n_type == IN_PIPE_BULK) ||
                (direction_n_type == IN_PIPE_INTERRUPT) ||
                (direction_n_type == IN_PIPE_ISOCHRONOUS))
            {
                /* In Iso and Non Iso */
                x2 = (3167 + (((7 * 8 * max_packet_size) / 6) * 1000UL));
                x2 /= 1000UL;
                x2 = 67667UL * x2;
                x2 /= 100UL;
                return (64060UL + (2 * NU_USB_HUB_LS_SETUP_DELAY) + x2 +
                        NU_USB_HOST_DELAY + 500UL) / 1000UL;

            }
        default:
            return 0L;          /* to make compiler happy ! */
    }
}

/*************************************************************************
*
* FUNCTION
*     usb_set_config
*
* DESCRIPTION
*     This function calculates the bandwidth requirements of the specified
*     configuration and if found acceptable sends SET_CONFIGURATION command
*     to the device.
*
* INPUTS
*     cb     pointer to stack control block.
*     dev    pointer to device control block.
*     bus    the bus associated with the device.
*     cnfg   pointer to the parsed descriptors structure of the device.
* OUTPUTS
*     STATUS
*     NU_SUCCESS          Indicates that the device has been successfully
*                         configured.
*     NU_USB_NO_BANDWIDTH Indicates that the configurations B/W
*                         requirements couldn't be met.
*     NU_USB_INVLD_ARG    Indicates that the cnfg passed is NU_NULL
*                         but the device specified is not yet configured.
*
*************************************************************************/
STATUS usb_set_config (NU_USBH_STACK * cb,
                       NU_USB_DEVICE * dev,
                       USBH_BUS_RESOURCES * bus,
                       NU_USB_CFG * cnfg)
{
    UINT32           len, total_load = 0;
    STATUS           status = NU_SUCCESS;
    STATUS           internal_sts = NU_SUCCESS;
    UINT8            i;
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_FALSE )
    UINT32           alt_set_load = 0;
    UINT8            j, interface_no;
    NU_USB_ALT_SETTG *alt_set;
    NU_USB_ENDP_DESC *ep;
    UINT8            direction_n_type;
    UINT16           max_pkt_size;
    NU_USB_HW        *hw_ctrl;
    USB_DEV_CURRENT_INFO current_requirment;
#endif


    if(cb == NU_NULL
       ||dev == NU_NULL
       ||bus == NU_NULL
       ||cnfg == NU_NULL)
    {
        return NU_USB_INVLD_ARG;
    }

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_FALSE )

    /* Check if enough current is avaiable to fulfill the device requirements. */
    hw_ctrl = dev->hw;
    current_requirment.device = dev;
    current_requirment.cfg = (UINT8) (cnfg->desc->bConfigurationValue - 1);
    current_requirment.is_current_available = NU_FALSE;

    /* Call hardware controller driver IOCTL. */
    status = DVC_Dev_Ioctl(hw_ctrl->dv_handle,
                           (hw_ctrl->ioctl_base_addr + NU_USB_IOCTL_IS_CURR_AVAILABLE),
                           (VOID*) &current_requirment,
                           sizeof(USB_DEV_CURRENT_INFO));

    if(status == NU_SUCCESS)
    {
        if(!current_requirment.is_current_available)
        {
            return NU_USB_NO_POWER;
        }
    }
    else
    {
        return status;
    }
#endif

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

    /* Check the BW availability in the Host controller for new
     * configuration.
     */
    status = NU_USBH_HW_Allocate_Bandwidth(cb, dev, cnfg,
                                        NU_NULL, NU_NULL);

    if ( status == NU_SUCCESS)
    {
        /* B/W requirements could be met, so reserve the required b/w and
         * send a SET_CONFIGURATION command to the device.
         */
        internal_sts = USBH_FILL_CNTRLPKT ((cb->ctrl_irp),
                            (USB_REQ_RECP_DEV | USB_REQ_STD | USB_REQ_OUT),
                            USB_SET_CONFIGURATION,
                            cnfg->desc->bConfigurationValue, 0, 0);
        NU_UNUSED_PARAM(internal_sts);

        len = usb_control_transfer (cb, &dev->ctrl_pipe, cb->ctrl_irp,
                                    NU_NULL, 0);

        if (len == 0)
            return NU_USB_TRANSFER_FAILED;
    }

#else

    /* Find the b/w requirements of this configuration. */
    for (interface_no = 0;
         interface_no < cnfg->desc->bNumInterfaces; interface_no++)
    {
        alt_set = &(cnfg->intf[interface_no].alt_settg[0]);
        if (alt_set->desc)
        {                   /* Valid alt setting */
            alt_set_load = 0;
            for (j = 0; j < NU_USB_MAX_ENDPOINTS; j++)
            {
                ep = alt_set->endp[j].desc;
                if (ep)
                {           /* Valid endpoint */
                    direction_n_type =
                        (UINT8)usb_calc_direction_n_type (ep->bEndpointAddress,
                                                   ep->bmAttributes);
                    max_pkt_size = ep->wMaxPacketSize0;
                    max_pkt_size |= (((UINT16)ep->wMaxPacketSize1) << 8);
                    alt_set->endp[j].load =
                        usb_calc_load (direction_n_type, dev->speed,
                                       /* Mult times Max Pkt Size */
                                       (UINT16)((1 +
                                       (((max_pkt_size) >> 11) & 0x03)) *
                                       (max_pkt_size & 0x3ff)));
                    alt_set_load += alt_set->endp[j].load;
                }
            }
        }
        alt_set->load = alt_set_load;
        total_load += alt_set_load;
    }

    if (bus->avail_bandwidth < total_load)
    {
        return NU_USB_NO_BANDWIDTH;
    }
    else
    {
        /* B/W requirements could be met, so reserve the required b/w and
         * send a SET_CONFIGURATION command to the device.
         */
        internal_sts = USBH_FILL_CNTRLPKT ((cb->ctrl_irp),
                            (USB_REQ_RECP_DEV | USB_REQ_STD | USB_REQ_OUT),
                            USB_SET_CONFIGURATION,
                            cnfg->desc->bConfigurationValue, 0, 0);
        NU_UNUSED_PARAM(internal_sts);

        len = usb_control_transfer (cb, &dev->ctrl_pipe, cb->ctrl_irp,
                                    NU_NULL, 0);

        if (len == 0)
            return NU_USB_TRANSFER_FAILED;
    }

    bus->avail_bandwidth -= total_load;

#endif

    /* Open pipes required on each of the interfaces. */
    if ( status == NU_SUCCESS )
    {
        for (i = 0; i < cnfg->desc->bNumInterfaces; i++)
        {
            status =  usb_set_interface_id (cb, dev, bus, cnfg, i, 0, NU_FALSE);
            if (status != NU_SUCCESS)
            {
                break;
            }
        }
    }

    if (status == NU_SUCCESS)
    {
        cnfg->load += total_load;
        cnfg->is_active = NU_TRUE;
        dev->state = USB_STATE_CONFIGURED;
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*      usb_unset_config
*
* DESCRIPTION
*    This function un-configures the configured device by sending a
* SET_CONFIGURATION command with configuration value of 0. It also closes all
* the opened pipes.
*
* INPUTS
*   cb ptr to stack control block.
*   dev Ptr to device control block.
*   bus the bus associated with the device.
*
* OUTPUTS
*   VOID
*
*************************************************************************/
STATUS usb_unset_config (NU_USBH_STACK * cb,
                         NU_USB_DEVICE * dev,
                         USBH_BUS_RESOURCES * bus)
{
    STATUS status = NU_SUCCESS;
    NU_USB_HW         *hw_ctrl;
    USB_DEV_CURRENT_INFO current_requirment;
    UINT32 len = 0;

    if(cb == NU_NULL
       ||dev == NU_NULL
       ||bus == NU_NULL)
    {
        return NU_USB_INVLD_ARG;
    }

    /* Release power from the controller. */
    hw_ctrl = dev->hw;
    current_requirment.device = dev;

    status = DVC_Dev_Ioctl(hw_ctrl->dv_handle,
                                 (hw_ctrl->ioctl_base_addr + NU_USB_IOCTL_RELEASE_POWER),
                                 (VOID*) &current_requirment,
                                 sizeof(USB_DEV_CURRENT_INFO));

    if ( status == NU_SUCCESS )
    {
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

        /* Release the active configuration. */
        status = usb_close_pipes (dev, bus, NU_FALSE);

        if ( status == NU_SUCCESS )
        {
            /* Unset the configuaation at device side. */
            status = USBH_FILL_CNTRLPKT ((cb->ctrl_irp),
                                (USB_REQ_RECP_DEV | USB_REQ_STD | USB_REQ_OUT),
                                USB_SET_CONFIGURATION, 0, 0, 0);
            if ( status == NU_SUCCESS )
            {
                len = usb_control_transfer(cb, &dev->ctrl_pipe, cb->ctrl_irp, NU_NULL, 0);
                if ( len == 0 )
                {
                    status = NU_USB_TRANSFER_FAILED;
                }
                else
                {
                    dev->state = USB_STATE_ADDRESSED;
                }
            }
        }
#else

        /* Send SET_CONFIGURATION command */
        status = USBH_FILL_CNTRLPKT ((cb->ctrl_irp),
                            (USB_REQ_RECP_DEV | USB_REQ_STD | USB_REQ_OUT),
                            USB_SET_CONFIGURATION, 0, 0, 0);
        if ( status == NU_SUCCESS )
        {
            len = usb_control_transfer(cb, &dev->ctrl_pipe, cb->ctrl_irp, NU_NULL, 0);

            dev->state = USB_STATE_ADDRESSED;

            NU_UNUSED_PARAM(len);

            /* close all the opened pipes in the h/w */
            status = usb_close_pipes (dev, bus, NU_FALSE);
        }
#endif
    }

    return ( status );
}

/*************************************************************************
*
* FUNCTION
*      usb_modify_ep
*
* DESCRIPTION
*    This function sends Endpoint Halt Clear/Set as per the 3rd argument
*  of this function.
* INPUTS
*    cb  ptr to stack control block.
*    dev Ptr to the device control block.
*    pipe  Ptr to pipe that identifies the endpoint to
*                        halted/cleared.
*    feature   set/clear endpoint halt. USB_SET_FEATURE/USB_CLEAR_FEATURE
*
* OUTPUTS
*     STATUS   NU_SUCCESS   Indicates that successful completion.
*              NU_USB_INVLD_ARG Indicates that the device is yet un-configured.
*
*************************************************************************/
STATUS usb_modify_ep (NU_USBH_STACK * cb,
                      NU_USB_DEVICE * dev,
                      NU_USB_PIPE * pipe,
                      UINT8 feature)
{
    STATUS internal_sts = NU_SUCCESS;
    if(cb == NU_NULL
       ||dev == NU_NULL
       ||pipe == NU_NULL)
    {
        return NU_USB_INVLD_ARG;
    }

    if ((pipe->endpoint == NU_NULL) ||
        (dev->active_cnfg_num == NU_USB_MAX_CONFIGURATIONS))
    {
        /* Device not yet configured */
        return NU_USB_INVLD_ARG;
    }

    /* Send Set/Clear feature command */
    internal_sts = USBH_FILL_CNTRLPKT ((cb->ctrl_irp),
                        (USB_REQ_RECP_EP | USB_REQ_STD | USB_REQ_OUT),
                        feature, USB_EP_HALT,
                        (pipe->endpoint->desc->bEndpointAddress), 0);

    NU_UNUSED_PARAM(internal_sts);
    if (usb_control_transfer (cb, &dev->ctrl_pipe, cb->ctrl_irp, NU_NULL, 0))
    {
        /* Allow the modification to settle down at the device */
        usb_wait_ms (100);
        return (NU_SUCCESS);
    }
    else
    {
        return NU_USB_TRANSFER_FAILED;
    }
}

/*************************************************************************
*
* FUNCTION
*      usb_verify_pipe
*
* DESCRIPTION
*     This functions looks up in the device's endpoint descriptor database, to
*  find if the endpoint specified in the pipe is a valid one.
* INPUTS
*    dev Ptr to the device.
*    pipe  Ptr to pipe that identifies the endpoint that is to
*                         be verified.
* OUTPUTS
*    NU_USB_ENDP_DESC * Ptr to the endpoint descriptor structure corresponding
*                         to the endpoint specified in the pipe. NU_NULL if
*                         no endpoint descriptor corresponds to the specified
*                         endpoint.
*
*************************************************************************/
NU_USB_ENDP_DESC *usb_verify_pipe (NU_USB_DEVICE * dev,
                                   NU_USB_PIPE * pipe)
{
    UINT8 i, j;
    NU_USB_ALT_SETTG *alt_set;
    NU_USB_ENDP_DESC *ep;

    if(dev == NU_NULL
       ||pipe == NU_NULL)
    {
        return NU_NULL;
    }

    if (dev->active_cnfg_num == NU_USB_MAX_CONFIGURATIONS)
    {
        /* Device not yet configured */
        return NU_NULL;
    }
    /* Look in the descriptor database of the device for this endpoint */
    for (i = 0; i < NU_USB_MAX_INTERFACES; i++)
    {
        alt_set =
            dev->config_descriptors[dev->active_cnfg_num]->intf[i].current;
        if (alt_set == NU_NULL)
            continue;
        for (j = 0; j < NU_USB_MAX_ENDPOINTS; j++)
        {
            ep = alt_set->endp[j].desc;
            if ((ep)
                && (pipe->endpoint->desc->bEndpointAddress ==
                    ep->bEndpointAddress))
            {
                return ep;
            }
        }
    }
    return NU_NULL;
}

/*************************************************************************
*
* FUNCTION
*     usb_fetch_string
*
* DESCRIPTION
*   It gets the string descriptor from the device.
*
* INPUTS
*     cb  ptr to stack control block.
*     dev ptr to device control block.
*     index  index of the string.
*
* OUTPUTS
*    STATUS   NU_SUCCESS Indicates that the string has been successfully
*               fetched.
*             NU_USB_INTERNAL_ERROR Indicates that the control transfer
*               failed to fetch the string descriptor from the device.
*
*************************************************************************/
STATUS usb_fetch_string_desc (NU_USBH_STACK * cb,
                              NU_USB_DEVICE * dev,
                              UINT8 index)
{
    UINT8 *buf;
    UINT16 lang_id;
    STATUS status = NU_SUCCESS;
    STATUS internal_sts = NU_SUCCESS;
    UINT16 string_id, j, num_lang_ids = 0;
    UINT32 len;
    UINT32 irp_len;

    if(cb == NU_NULL
       ||dev == NU_NULL)
    {
        return NU_USB_INVLD_ARG;
    }

    /* If not already done, get all supported lang_ids */
    if(dev->string_descriptors[0] == NU_NULL)
    {
        /* Allocate memory for this descriptor */
        status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                     sizeof(NU_USB_STRING),
                                     (VOID **) &dev->string_descriptors[0]);

        if (status != NU_SUCCESS)
            return (status);

        dev->string_descriptors[0]->str_index = 0;
        dev->string_descriptors[0]->wLangId = 0;

        memset(&dev->string_descriptors[0]->string[0], 0,
               NU_USB_MAX_STRING_LEN);

        internal_sts = USBH_FILL_CNTRLPKT ((cb->ctrl_irp),
                            (USB_REQ_RECP_DEV | USB_REQ_STD | USB_REQ_IN),
                            USB_GET_DESCRIPTOR, (USB_DT_STRING << 8), 0, 4);

        len = usb_control_transfer (cb, &dev->ctrl_pipe, cb->ctrl_irp,
                                    &dev->string_descriptors[0]->string[0],
                                    NU_USB_MAX_STRING_LEN);

        if (!len)
        {
            internal_sts |= USB_Deallocate_Memory(dev->string_descriptors[0]);
            NU_UNUSED_PARAM(internal_sts);

            dev->string_descriptors[0] = NU_NULL;
            return NU_USB_INTERNAL_ERROR;
        }

        if(dev->string_descriptors[0]->string[0] % dev->device_descriptor.bMaxPacketSize0)
        {
            irp_len = dev->string_descriptors[0]->string[0];
        }
        else
        {
            irp_len = 0xFF;
        }

        internal_sts |= USBH_FILL_CNTRLPKT ((cb->ctrl_irp),
                            (USB_REQ_RECP_DEV | USB_REQ_STD | USB_REQ_IN),
                            USB_GET_DESCRIPTOR, (USB_DT_STRING << 8), 0,
                            irp_len);

        len = usb_control_transfer (cb, &dev->ctrl_pipe, cb->ctrl_irp,
                                    &dev->string_descriptors[0]->string[0],
                                    irp_len);

        if (!len)
        {
            internal_sts |= USB_Deallocate_Memory(dev->string_descriptors[0]);
            NU_UNUSED_PARAM(internal_sts);

            dev->string_descriptors[0] = NU_NULL;
            return NU_USB_INTERNAL_ERROR;
        }
    }

    /* Check if this string descriptor already exists */
    for (string_id = 1; string_id < NU_USB_MAX_STRINGS; string_id++)
    {
        if ((dev->string_descriptors[string_id] != NU_NULL) &&
            (dev->string_descriptors[string_id]->str_index == index))
        {

            return (NU_SUCCESS);
        }
    }

    /* Find out how many languages are supported by this device `*/
#if (MULTI_LANGUAGES == NU_TRUE)
    num_lang_ids = (dev->string_descriptors[0]->string[0] - 2)/2;
#else
    num_lang_ids = ONE_LANGUAGE;
#endif

    buf = (UINT8 *)&dev->string_descriptors[0]->string[0];

    for(j = 0; j < num_lang_ids; j++)
    {
        lang_id = buf[j+2] | (buf[j+3] << 8);

        /* Locate an empty slot */
        for (string_id = 1; string_id < NU_USB_MAX_STRINGS; string_id++)
        {
            if (dev->string_descriptors[string_id] == NU_NULL)
            {
                break;
            }
        }

        /* make sure we have a valid index to place the string */
        if ( string_id >= NU_USB_MAX_STRINGS )
        {

            return NU_USB_MAX_EXCEEDED;
        }

        /* Allocate memory for this descriptor */
        status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                     sizeof(NU_USB_STRING),
                                     (VOID **)&dev->string_descriptors[string_id]);

        if (status != NU_SUCCESS)
            break;

        dev->string_descriptors[string_id]->str_index = index;
        dev->string_descriptors[string_id]->wLangId = lang_id;

        memset(&dev->string_descriptors[string_id]->string[0], 0,
               NU_USB_MAX_STRING_LEN);

        /* Get the size of string descriptor */
        internal_sts = USBH_FILL_CNTRLPKT ((cb->ctrl_irp),
                            (USB_REQ_RECP_DEV | USB_REQ_STD | USB_REQ_IN),
                             USB_GET_DESCRIPTOR, ((USB_DT_STRING << 8) + index),
                             lang_id, 4);

        len = usb_control_transfer (cb, &dev->ctrl_pipe, cb->ctrl_irp,
                              &dev->string_descriptors[string_id]->string[0],
                              NU_USB_MAX_STRING_LEN);

        if (!len)
        {
            internal_sts |= USB_Deallocate_Memory(dev->string_descriptors[string_id]);
            dev->string_descriptors[string_id] = NU_NULL;
        }
        else
        {
            if(dev->string_descriptors[string_id]->string[0] % dev->device_descriptor.bMaxPacketSize0)
            {
                irp_len = dev->string_descriptors[string_id]->string[0];
            }
            else
            {
                irp_len = 0xFF;
            }

            /* Get the string descriptor */
            internal_sts |= USBH_FILL_CNTRLPKT ((cb->ctrl_irp),
                            (USB_REQ_RECP_DEV | USB_REQ_STD | USB_REQ_IN),
                            USB_GET_DESCRIPTOR, ((USB_DT_STRING << 8) + index),
                            lang_id, irp_len);

            len = usb_control_transfer (cb, &dev->ctrl_pipe, cb->ctrl_irp,
                                  &dev->string_descriptors[string_id]->string[0],
                                  irp_len);

            if (!len)
            {
                /* We don't consider this as a HARD error. */
                internal_sts |= USB_Deallocate_Memory(dev->string_descriptors[string_id]);
                dev->string_descriptors[string_id] = NU_NULL;
            }
        }
    }



    /* Check if there are any memory errors.
     * if so free all the memory allocated and return error
     * */
    if(status != NU_SUCCESS)
    {
        for (string_id = 0; string_id < NU_USB_MAX_STRINGS; string_id++)
        {
            if (dev->string_descriptors[string_id] != NU_NULL)
            {
                internal_sts |= USB_Deallocate_Memory(dev->string_descriptors[string_id]);
                dev->string_descriptors[string_id] = NU_NULL;
            }
        }
    }

    return ((status == NU_SUCCESS) ? internal_sts : status);
}

/*************************************************************************
*
* FUNCTION
*     usbh_find_bus
*
* DESCRIPTION
*     It finds the array index of the bus associated with the device
*
* INPUTS
*     cb                 pointer to stack control block.
*     device             pointer to device control block.
*
* OUTPUTS
*     USBH_BUS_RESOURCES  pointer to the bus resources
*
*************************************************************************/
USBH_BUS_RESOURCES *usbh_find_bus (NU_USBH_STACK * stack,
                                   NU_USB_DEVICE * device)
{
    USBH_BUS_RESOURCES  *bus;
    UINT32              i = 0;

    if(stack == NU_NULL
       ||device == NU_NULL)
    {
        return NU_NULL;
    }

    /* find host controller to which given device is connected */
    while ((i < NU_USBH_MAX_HW) &&
           (device->hw != (NU_USB_HW *) stack->bus_resources[i].controller))
        i++;

    /* if there no such controller, returns NULL */
    if (i == NU_USBH_MAX_HW)
        return NU_NULL;

    /* get the bus pointer */
    bus = &(stack->bus_resources[i]);

    /* double check if the bus has a device with given address */
    if(usb_is_id_set(bus->dev_ids, USB_MAX_DEVID/8, device->function_address))
        return bus;
    else
        return NU_NULL;
}

/*************************************************************************
* FUNCTION
*        usbh_test_mode_enable_using_pid
*
* DESCRIPTION
*       This function enables appropriate host test mode that is based
*       on PID of the device attached to it.
*
*
* INPUTS
*       PID                     PID of connected device.
*
* OUTPUTS
*
*       NU_SUCCESS              Successful completion.
*
*************************************************************************/
#if (USB_TEST_MODE_SUPPORT == NU_TRUE)
STATUS usbh_test_mode_enable_using_pid(NU_USBH_STACK *cb,
                                       NU_USB_DEVICE *device, UINT16 PID)
{
    STATUS status = NU_SUCCESS;

    /* Enumerate device with Configuration Index 0 */
    status = usbh_test_mode_enumerate(cb, device, 0);

    if (status == NU_SUCCESS)
    {
        switch (PID)
        {
            case(NU_USBH_TEST_SE0_NAK_DEV_PID):
                status = NU_USBH_Test_Execute(device->hw, USB_TEST_SE0_NAK);
            break;

            case(NU_USBH_TEST_J_DEV_PID):
                status = NU_USBH_Test_Execute(device->hw, USB_TEST_J);
            break;

            case(NU_USBH_TEST_K_DEV_PID):
                status = NU_USBH_Test_Execute(device->hw, USB_TEST_K);
            break;

            case(NU_USBH_TEST_PACKET_DEV_PID):
                status = NU_USBH_Test_Execute(device->hw, USB_TEST_PACKET);
            break;

            case(NU_USBH_SUSPEND_RESUME_DEV_PID):
                /* Step 1: Keep sending SOF for another 15 seconds */
                NU_Sleep(NU_PLUS_TICKS_PER_SEC * 15);

                /* Step 2 Host Suspend */
                status = NU_USBH_STACK_Suspend_Device (cb, device);

                if (status == NU_SUCCESS)
                {
                    /* Step 3 Wait for 15 seconds in suspension state */
                    NU_Sleep(NU_PLUS_TICKS_PER_SEC * 15);

                    /* Step 4: Resume Host so that it may start sending SOF again */
                    status = NU_USBH_STACK_Resume_Device(cb,device);
                }
            break;

            case(NU_USBH_GET_DESC_DEV_PID):
                    /* Step 1: Keep sending SOF for another 15 seconds */
                    NU_Sleep(NU_PLUS_TICKS_PER_SEC * 15);

                    /* Step 2 Get Descriptor */
                    status = usbh_test_mode_get_descriptor(cb, device);
            break;

            case(NU_USBH_SET_FEATURE_DEV_PID):
                    /* Step 1 Get Device Descriptor */
                    status = usbh_test_mode_get_descriptor(cb, device);

                    /* Step 2: Keep sending SOF for another 15 seconds */
                    NU_Sleep(NU_PLUS_TICKS_PER_SEC * 15);

                    if (status == NU_SUCCESS)
                    {
                        /* Step 3: Issue an IN packet, simply request
                         * Device descriptor again */
                        status = usbh_test_mode_get_descriptor(cb, device);
                    }
            break;

            default:
                status = NU_USB_INVLD_ARG;
            break;
        }
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*     usbh_test_mode_get_descriptor
*
* DESCRIPTION
*     This function is used during testing of device in which
*     single step descriptor fetch is tested
*
* INPUTS
*     cb                 Pointer to stack control block.
*     dev                Pointer to device control block.
*
* OUTPUTS
*
*     NU_SUCCESS           Successful completion.
*     NU_USB_INVLD_ARG     Any of the input argument in invalid.
*     NU_USB_INVLD_DESC    Indicates that one of the descriptors returned
*                          by the device is invalid.
*
*************************************************************************/
STATUS usbh_test_mode_get_descriptor(NU_USBH_STACK *cb,
                           NU_USB_DEVICE *device)
{
    NU_USB_DEVICE_DESC  temp_buffer;
    UINT32 len = 0;
    STATUS status;

    /* Get Device Descriptor from Device */
    status = USBH_FILL_CNTRLPKT ((cb->ctrl_irp),
                        (USB_REQ_RECP_DEV | USB_REQ_STD | USB_REQ_IN),
                        USB_GET_DESCRIPTOR, (USB_DT_DEVICE << 8), 0,
                        sizeof (NU_USB_DEVICE_DESC));

    /* Zero out data IO buffer. */
    memset(&temp_buffer, 0, sizeof (NU_USB_DEVICE_DESC));

    len = usb_control_transfer (cb, &(device->ctrl_pipe), cb->ctrl_irp,
                            &temp_buffer,
                            sizeof (NU_USB_DEVICE_DESC));

    if (len == 0)
    {
        status = NU_USB_INVLD_DESC;
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*     usbh_test_mode_enumerate
*
* DESCRIPTION
*     This function is used to enumerate device in test mode for
*     executing any of the tests
*
* INPUTS
*     cb                    Pointer to stack control block.
*     device                Pointer to device control block.
*     config_index          Index for Configuration descriptor.
*
* OUTPUTS
*
*     NU_SUCCESS           Successful completion.
*     NU_USB_INVLD_ARG     Any of the input argument in invalid.
*     NU_USB_INVLD_DESC    Indicates that one of the descriptors returned
*                          by the device is invalid.
*
*************************************************************************/
STATUS usbh_test_mode_enumerate(NU_USBH_STACK *cb,
                           NU_USB_DEVICE *device,
                           UINT8 config_index)
{

    STATUS status;
    UINT32 len;
    NU_USB_CFG *config;

    /* get configuration descriptor (index 0 by default)*/
    config = device->config_descriptors[config_index];

    status = USBH_FILL_CNTRLPKT ((cb->ctrl_irp),
                        (USB_REQ_RECP_DEV | USB_REQ_STD | USB_REQ_OUT),
                        USB_SET_CONFIGURATION,
                        config->desc->bConfigurationValue, 0, 0);

    len = usb_control_transfer (cb, &device->ctrl_pipe, cb->ctrl_irp,
                                NU_NULL, 0);

    if (len == 0)
    {
        status = NU_USB_TRANSFER_FAILED;
    }

    return (status);
}

#endif

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
/*************************************************************************
*
* FUNCTION
*     USBH_Get_BOS_Descriptor
*
* DESCRIPTION
*     This function is used during the SuperSpeed device enumeration for
*     retrieving the BOS descriptor.
*
* INPUTS
*     cb                 Pointer to stack control block.
*     dev                Pointer to device control block.
*
* OUTPUTS
*
*     NU_SUCCESS           Successful completion.
*     NU_USB_INVLD_ARG     Any of the input argument in invalid.
*     NU_USB_INVLD_DESC    Indicates that one of the descriptors returned
*                          by the device is invalid.
*
*************************************************************************/
STATUS USBH_Get_BOS_Descriptor(NU_USBH_STACK *cb,
                               NU_USB_DEVICE *dev)
{
    STATUS          status          = NU_SUCCESS;
    STATUS          internal_status = NU_SUCCESS;
    NU_USB_PIPE     *pipe           = NU_NULL;
    NU_USB_BOS_DESC *desc           = NU_NULL;
    UINT16          bos_length      = 0;
    UINT16          temp_length     = 0;
    UINT8           *temp_buffer    = NU_NULL; /* buffer for holding first 5
                                                  bytes of BOS descriptor. */
    UINT8           rollback        = 0;

    /* Parameters validation. */
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(dev);

    /* Allocate uncached memory buffer for data IO. */
    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                 5,
                                 (VOID **) &temp_buffer);
    if(status != NU_SUCCESS)
    {
        return (status);
    }

    desc = (NU_USB_BOS_DESC *) temp_buffer;
    pipe = &(dev->ctrl_pipe);

    do
    {
        /* Fetch first 5 bytes of BOS descriptor in order to know about
         * the length of complete BOS descriptor.
         */
        USBH_FILL_CNTRLPKT ((cb->ctrl_irp),
                           (USB_REQ_RECP_DEV | USB_REQ_STD | USB_REQ_IN),
                           USB_GET_DESCRIPTOR, (USB_DT_BOS << 8) ,0,5);

        /* Zero out newly allocated buffer. */
        memset(temp_buffer, 0, 5);

        temp_length = usb_control_transfer (cb, pipe, (cb->ctrl_irp),
                                            temp_buffer, 5);

        if (temp_length < 5)
        {
            status = NU_USB_INVLD_DESC;
            break;
        }

        /* Convert to CPU's endianness */
        bos_length = LE16_2_HOST (desc->wTotalLength);

        /* Allocate memory for complete BOS descriptor.*/
        status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                     bos_length,
                                    (VOID **) &(dev->raw_bos_descriptors));

        if (status != NU_SUCCESS)
        {
            break;
        }

        memset (dev->raw_bos_descriptors, 0, bos_length);

        /* Read the complete BOS descriptor.*/
        USBH_FILL_CNTRLPKT ((cb->ctrl_irp),
                            (USB_REQ_RECP_DEV | USB_REQ_STD | USB_REQ_IN),
                            USB_GET_DESCRIPTOR, (USB_DT_BOS << 8), 0,
                            bos_length);

        temp_length = usb_control_transfer (cb, pipe, (cb->ctrl_irp),
                                            dev->raw_bos_descriptors,
                                            bos_length);

        if (temp_length < bos_length)
        {
            status = NU_USB_INVLD_DESC;
            rollback = 1;
            break;
        }

        status = USB_Allocate_Object(sizeof (NU_USB_BOS),
                                     (VOID **)&(dev->bos));
        if (status != NU_SUCCESS)
        {
            rollback = 1;
            break;
        }

        memset (dev->bos, 0, sizeof(NU_USB_BOS));

        /* Parse the BOS descriptors */
        status = USB_Parse_BOS_Descriptor (dev,
                                           dev->raw_bos_descriptors,
                                           dev->bos, bos_length);

        if (status != NU_SUCCESS)
        {
            rollback = 2;
        }

    } while (0);

    /* If something went wrong then rollback. */
    switch(rollback)
    {
        case 2:
            if (dev->bos)
            {
               internal_status = USB_Deallocate_Memory (dev->bos);
               dev->bos = NU_NULL;
            }

        case 1:
            if(dev->raw_bos_descriptors)
            {
               internal_status = USB_Deallocate_Memory (
                                               dev->raw_bos_descriptors);
               dev->raw_bos_descriptors = NU_NULL;
            }

        default:
            /* Deallocate temporary buffer now. */
            internal_status = USB_Deallocate_Memory(temp_buffer);

            NU_UNUSED_PARAM(internal_status);
            return status;
    }
}

/*************************************************************************
*
* FUNCTION
*     USBH_Set_Isochronous_Delay
*
* DESCRIPTION
*     This function is used to set the isochronous delay during
*     SuperSpeed device enumeration.
*
* INPUTS
*     cb                 Pointer to stack control block.
*     dev                Pointer to device control block.
*
* OUTPUTS
*
*     NU_SUCCESS             Successful completion.
*     NU_USB_INVLD_ARG       Any of the input arguments is in invalid.
*     NU_USB_TRANSFER_FAILED Transfer is not completed successfully.
*     NU_USB_INVLD_HUB       Hub device pointer is NU_NULL.
*
*************************************************************************/
STATUS USBH_Set_Isochronous_Delay(NU_USBH_STACK *cb,
                                  NU_USB_DEVICE *dev)
{
    STATUS              status      = NU_SUCCESS;
    NU_USBH_DRVR_HUB    *hub_drvr   = NU_NULL;
    NU_USBH_DEVICE_HUB  *hub        = NU_NULL;
    NU_USB_PIPE         *pipe       = NU_NULL;
    UINT16              length      = 0;
    UINT16              isoch_delay = 0;

    /* Parameters validation. */
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(dev);

    /* Get the hub driver pointer from the stack control block. */
    hub_drvr = &(cb->hub_driver);
    pipe = &(dev->ctrl_pipe);

    if ( !USBH_IS_ROOT_HUB(dev) )
    {
        /* Find the hub where device is attached.*/
        hub = (NU_USBH_DEVICE_HUB *) usb_find_hub(hub_drvr, dev->parent);

        if (hub == NU_NULL)
        {
            status = NU_USB_INVLD_HUB;
        }

        if (status == NU_SUCCESS)
        {
            /* Get the value of isochronous delay. */
            isoch_delay = hub->isoch_delay;
            if ( isoch_delay )
            {
                /* Fill the setup packet for SET_ISOCH_REQUEST. */
                USBH_FILL_CNTRLPKT ((cb->ctrl_irp),
                               (USB_REQ_RECP_DEV | USB_REQ_STD | USB_REQ_OUT),
                               SET_ISOCH_DELAY, isoch_delay ,0,0);

                /* Transfer isochronous delay value to the device.*/
                length = usb_control_transfer (cb, pipe, (cb->ctrl_irp),
                                               NU_NULL, 0);

                if (length == 0)
                {
                    status = NU_USB_TRANSFER_FAILED;
                }
            }

        }
    }
    return status;
}

/*************************************************************************
*
* FUNCTION
*     USBH_Set_System_Exit_Latency
*
* DESCRIPTION
*     This function is used for setting the System Exit Latency (SEL)
*     during SuperSpeed device enumeration.
*
* INPUTS
*     cb                 Pointer to stack control block.
*     dev                Pointer to device control block.
*
* OUTPUTS
*
*     NU_SUCCESS             Successful completion.
*     NU_USB_INVLD_ARG       Any of the input arguments is in invalid.
*     NU_USB_TRANSFER_FAILED Trnasfer is not completed successfully.
*
*************************************************************************/
STATUS USBH_Set_System_Exit_Latency(NU_USBH_STACK *cb,
                                    NU_USB_DEVICE *dev)
{
    STATUS               status       = NU_SUCCESS;
    NU_USB_PIPE          *pipe        = NU_NULL;
    NU_USB_DEVICE_SEL    *sel         = NU_NULL;
    NU_USB_DEV_PW_ATTRIB *pwr_attrib  = NU_NULL;
    UINT8                *temp_buffer = NU_NULL;
    UINT16               length       = 0;

    /* Parameters validation.*/
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(dev);

    pipe = &(dev->ctrl_pipe);
    /* Get the power attributes of the device. */
    status = NU_USB_PMG_Get_Dev_Pwr_Attrib(dev, &pwr_attrib);

    if (status == NU_SUCCESS)
    {
        sel = &(pwr_attrib->sel);

        /* Call to usb_calc_system_exit_latency() function. This function
         * calculates the components of System Exit Latency (SEL) and save
         * them in the NU_USB_DEVICE_SEL sel field of NU_USB_DEV_PW_ATTRIB
         * structure.The sel field is subsequently used by the control
         * transfer
         */

        status = NU_USB_PMG_Calculate_SEL(dev, sel);
    }

    if (status == NU_SUCCESS)
    {
        /* Allocate uncached memory buffer for data IO. */
        status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                     6,
                                     (VOID **) &temp_buffer);
        if(status == NU_SUCCESS)
        {
            /* Fill the setup packet for SET_SEL request. */
            USBH_FILL_CNTRLPKT ((cb->ctrl_irp),
                           (USB_REQ_RECP_DEV | USB_REQ_STD | USB_REQ_OUT),
                           SET_SEL, 0, 0, 6);

            /* Transfer U1 and U2 system exit latency parameters to the
             * device.
             */
            length = usb_control_transfer (cb, pipe, (cb->ctrl_irp),
                                           temp_buffer, 6);
            if (length == 0)
            {
                status =  NU_USB_TRANSFER_FAILED;
            }
            else
            {
                memcpy(sel, temp_buffer, 6);
            }

            /* De-allocate temporary buffer now. */
            status = USB_Deallocate_Memory(temp_buffer);
        }
    }

    return status;
}

/*************************************************************************
* FUNCTION
*        USBH_Modify_Interface_Feature
*
* DESCRIPTION
*       This function sends set/cleare_feature request to the interface
*       in the SuperSpeed device.
*
* INPUTS
*       cb               Pointer to stack control block.
*       device           Pointer to device control block.
*       intf             Pointer to interface control block.
*       feature          Pointer to NU_USB_INTF control block.
*       options          Suspend options encoded in Bit 0 and 1. Please
*                        Refer to Set_Feature(Function_Suspend) function
*                        in Chapter 9 of USB 3.0 specifications.
*       feature_selector Feature selector code.
*
* OUTPUTS
*
*     NU_SUCCESS             Successful completion.
*     NU_USB_INVLD_ARG       Any of the input arguments is in invalid.
*     NU_USB_TRANSFER_FAILED Transfer is not completed successfully.
*
*************************************************************************/
STATUS USBH_Modify_Interface_Feature(NU_USBH_STACK   *cb,
                                     NU_USB_DEVICE   *dev,
                                     NU_USB_INTF     *intf,
                                     UINT8           feature,
                                     UINT8           options,
                                     UINT8           feature_selector)
{
    STATUS      status     = NU_SUCCESS;
    NU_USB_PIPE *pipe      = NU_NULL;
    UINT16      w_index    = 0;
    UINT8       length     = 0;

    /* Parameters validation.*/
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(dev);
    NU_USB_PTRCHK(intf);

    w_index = (intf->intf_num << 8) | options ;
    pipe = &(dev->ctrl_pipe);

    /* Fill the control packet for set/clear_feature(Function_Suspend)
     * request.
     */
    status = USBH_FILL_CNTRLPKT ((cb->ctrl_irp),
                          (USB_REQ_RECP_INTF | USB_REQ_STD | USB_REQ_OUT),
                          feature, feature_selector, w_index, 0);

    if (status == NU_SUCCESS)
    {
       length = usb_control_transfer (cb, pipe, cb->ctrl_irp, NU_NULL, 0);

       if (length == 0)
       {
           status = NU_USB_TRANSFER_FAILED;
       }
    }

    return status;
}

/*************************************************************************
* FUNCTION
*        USBH_Modify_Device_Feature
*
* DESCRIPTION
*       This function sends set/cleare_feature request to the SuperSpeed
*       device for device level features.
*
*
* INPUTS
*       cb                Pointer to stack control block.
*       device            Pointer to device control block.
*       feature           bRequest, set_feature or clear_feature.
*       feature_selector  Feature selector code.
*
* OUTPUTS
*
*     NU_SUCCESS              Successful completion.
*     NU_USB_INVLD_ARG        Any of the input arguments is in invalid.
*     NU_USB_TRANSFER_FAILED  Transfer is not completed successfully.
*
*************************************************************************/
STATUS USBH_Modify_Device_Feature(NU_USBH_STACK  *cb,
                                  NU_USB_DEVICE  *dev,
                                  UINT8           feature,
                                  UINT8           feature_selector)
{
    STATUS      status     = NU_SUCCESS;
    STATUS      int_status = NU_SUCCESS;
    NU_USB_PIPE *pipe      = NU_NULL;
    BOOLEAN     unlock     = NU_FALSE;
    UINT8       length     = 0;

    /* Parameters validation.*/
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(dev);

    pipe = &(dev->ctrl_pipe);

    /* Lock the device for exclusive access. */
    status = NU_USB_DEVICE_Lock(dev);

    if (status == NU_SUCCESS)
    {
        /* Fill the control packet for set/clear_feature request for
         * device level features like U1_ENABLE,U2_ENABLE and LTM_ENABLE.
         */
        status = USBH_FILL_CNTRLPKT ((cb->ctrl_irp),
                          (USB_REQ_RECP_DEV | USB_REQ_STD | USB_REQ_OUT),
                          feature, feature_selector, 0, 0);
        unlock = NU_TRUE;
    }

    if (status == NU_SUCCESS)
    {
       length = usb_control_transfer (cb, pipe, cb->ctrl_irp, NU_NULL, 0);

       if (length == 0)
       {
           status = NU_USB_TRANSFER_FAILED;
       }
    }

    if (unlock == NU_TRUE)
    {
        int_status = NU_USB_DEVICE_Unlock(dev);
    }

    return ((status == NU_SUCCESS) ? int_status : status);
}

#endif      /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */
/************************************************************************/

#endif /* USBH_STACK_IMP_C */
/* ======================  End Of File  =============================== */

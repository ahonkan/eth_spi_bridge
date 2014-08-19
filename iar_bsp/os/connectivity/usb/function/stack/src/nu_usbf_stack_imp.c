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

***************************************************************************
*
* FILE NAME 
*
*       nu_usbf_stack_imp.c
*
*
* COMPONENT
*
*       Stack component / Nucleus USB Device Software.
*
* DESCRIPTION
*
*       Contains the implementation of the APIs exported by the USB
*       Function Stack. These APIs are exported through the Base USB
*       Stack component.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       usbf_enable_device                  Enables a function controller.
*       usbf_disable_driver                 Disables a class driver.
*       usbf_disable_device                 Disables a function controller.
*       usbf_get_device                     Retriers the device for an FC.
*       usbf_validate_setup                 Validates a setup packet from
*                                           device state.
*       usbf_ep0_transfer_complete          Transfer completion callback
*                                           for EP 0.
*       usbf_validate_endpoint              Validates an endpoint from
*                                           descriptors.
*       usbf_validate_interface             Validates an interface from
*                                           descriptors.
*       usbf_process_class_specific         Class specific SETUP processing
*       usbf_request_error                  Invalid / Unsupported request
*                                           processing.
*       usbf_get_status                     GET STATUS request processing.
*       usbf_feature                        G(S)ET FEATURE request
*                                           processing.
*       usbf_set_address                    SET ADDRESS request processing.
*       usbf_get_descriptor                 GET DESCRIPTOR request
*                                           processing.
*       usbf_set_descriptor                 SET DESCRIPTOR request
*                                           processing.
*       usbf_get_interface                  GET INTERFACE request
*                                           processing.
*       usbf_set_interface                  SET INTERFACE request
*                                           processing.
*       usbf_get_configuration              GET CONFIGURATION request
*                                           processing.
*       usbf_set_configuration              SET CONFIGURATION request
*                                           processing.
*       usbf_sync_frame                     SYNC FRAME request processing.
*       usbf_setup_descriptor               Finds descriptors for the 
*                                           operating speed.
* DEPENDENCIES
*
*       nu_usb.h
*
**************************************************************************/
#ifndef     USBF_STACK_IMP_C
#define     USBF_STACK_IMP_C

/* ==============  USB Include Files ==================================  */

#include    "connectivity/nu_usb.h"

/**************************************************************************
*
* FUNCTION
*
*       usbf_enable_device
*
* DESCRIPTION
*
*       This function aims to find a) a device level class driver for this
*       device or b) Interface level drivers for each of the interfaces in
*       this device, in the active configuration.
*
*       Finding drivers involves in the following steps :
*
*       For each of the class drivers, invoke the examine device
*       callback. if it succeeds then invoke initialize device
*       callback. By this time, the driver must have claimed the
*       device. If it has not found, move on to next driver.
*
*       If no device level driver is found, then its turn to lookout
*       for interface level drivers. For each of the interfaces in the
*       device, repeat the same procedure listed above, except that
*       examine device callback will be replaced by examine interface
*       and initialize device will be replaced by initialize interface
*       callbacks.
*
* INPUTS
*
*       device                              The USB device for which the
*                                           drivers are to be found.
*
* OUTPUTS
*
*       NU_SUCCESS
*       NU_USB_INVLD_ARG    Indicates the one of the input
*                           parameters has been incorrect
*                           AND/OR the event could not be
*                           processed without an error.
**************************************************************************/
STATUS  usbf_enable_device (NU_USBF_DEVICE *device)
{
    NU_USB_STACK   *cb;
    NU_USB_DRVR    *next;
    USB_DRVR_LIST  *current;
    USB_DRVR_LIST  *next_node;
    STATUS          status = NU_SUCCESS;
    INT             i;

    /* Error checks. */
    if (device  == NU_NULL)
    {
        return NU_USB_INVLD_ARG;
    }
    cb  = device->usb_device->stack;
    current = cb->class_driver_list_head;

    if (cb->num_class_drivers == 0)
    {
        return (NU_SUCCESS);
    }
    next = current->driver;

    /* First, find if there are any device level drivers. */
    next_node = current;

    while (next)
    {
        status = NU_USB_DRVR_Examine_Device (next, &(device->usb_device->
                                               device_descriptor));
        if (status == NU_SUCCESS)
        {
            /* Try this driver. */
            status = NU_USB_DRVR_Initialize_Device (next,
                                                  cb, device->usb_device); 

            if (device->usb_device->driver)
            {
                return (NU_SUCCESS);
            }
        }

        next_node = (USB_DRVR_LIST *) (next_node->list_node.cs_next);
        next = next_node->driver;

        /* End of the class drivers list ?? */
        if (next == cb->class_driver_list_head->driver)
        {
            next = NU_NULL;
        }
    }

    /* No driver is a device level driver OR
     * no driver is prepared to accept this device at the device level.
     * look out for interface level drivers.
     */

    /* If the device has not been configured yet, ignore the device.
     */
    if (device->active_config == NU_NULL)
    {
        return (NU_SUCCESS);
    }
    /* For each of the interfaces, we search for all available drivers. */

    for (i = 0; i < device->active_config->desc->bNumInterfaces; i++)
    {
        current = cb->class_driver_list_head;
        next = current->driver;
        next_node = current;

        while (next)
        {
            /* Check if this driver accepts this device. */
            status = NU_USB_DRVR_Examine_Intf (next,
                             device->active_config->intf[i].current->desc);

            /* Does this accept ? */
            if (status == NU_SUCCESS)
            {
                /* Try using this driver. */
                status = NU_USB_DRVR_Initialize_Interface (next, cb,
                                      device->usb_device,
                                      &device->active_config->intf[i]);

                if (device->active_config->intf[i].driver)
                {
                    break;
                }
            }

            /* Go to next driver. */
            next_node = (USB_DRVR_LIST *) (next_node->list_node.cs_next);
            next = next_node->driver;

            /* End of the class drivers list ?? */
            if (next == cb->class_driver_list_head->driver)
            {
                next = NU_NULL;
            }
        }

        /* Off to next interface. */
    }

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       usbf_disable_driver
*
* DESCRIPTION
*
*       This function checks to see if the given driver is actively serving
*       any devices / interfaces contained within.
*
* INPUTS
*
*       stack                               Stack control block.
*       driver                              Class driver control block to
*                                           be enabled.
*
* OUTPUTS
*
*       NU_SUCCESS                          If the driver doesn't own any
*                                           of the interfaces / devices.
*       NU_USB_INVLD_ARG                    If the driver is owning any of
*                                           the interfaces / devices.
*
**************************************************************************/
STATUS  usbf_disable_driver (NU_USBF_STACK *stack,
                             NU_USB_DRVR   *driver)
{
    NU_USBF_DEVICE *device;
    UINT16             i;
    UINT16             j;

    /* Error checking.   */
    if (stack == NU_NULL || driver == NU_NULL)
    {
        return  NU_USB_INVLD_ARG;
    }
     /* The purpose of this function is to check if there are any 'devices'
     * or 'interfaces' that this driver is serving.
     * De-registration of such 'active' drivers must not be allowed.
     */
    for (i = 0; i < stack->num_devices; i++)
    {
        device = &(stack->device_list[i]);

        /* Check if this is the device  level driver for this device. */
        if (device->usb_device->driver == driver)
        {
            return (NU_USB_INVLD_ARG);
        }

        /* Check if this is the interface   level driver for any of the
         * interfaces of this device.
         */

        /* If the device is not configured yet, ignore it. */
        if (device->active_config == NU_NULL)
        {
            continue;
        }
        for (j = 0; j < device->active_config->desc->bNumInterfaces; j++)
        {
            if (device->active_config->intf[j].driver == driver)
            {
                return (NU_USB_INVLD_ARG);
            }
        }
        /* Off to the next device. */
    }

    return (NU_SUCCESS);
}

/**************************************************************************
*
* FUNCTION
*
*       usbf_disable_device
*
* DESCRIPTION
*
*       This function issues a disable callback for all the class drivers
*       on a specific FC.
*
*       It checks through all the device level driver and interface level
*       drivers and issues a disconnect callback for each of them.
*
* INPUTS
*
*       device                              Device to be disabled.
*
* OUTPUTS
*
*       NU_SUCCESS                          If the driver doesn't own any
*                                           of the interfaces / devices.
*       NU_USB_INVLD_ARG                    IF the device pointer is 
*                                           invalid.
*
**************************************************************************/
STATUS    usbf_disable_device (NU_USBF_DEVICE *device)
{
    INT             j = 0;
    NU_USB_DEVICE  *usb_device = NU_NULL;
    STATUS         status = NU_SUCCESS;
    
    /* Error checking.   */
    if (device == NU_NULL)
    {
        return NU_USB_INVLD_ARG;
    } 
    
    usb_device = device->usb_device;
    device->state = USB_STATE_DEFAULT;

    /* Check if there is a device  level driver for this device. */
    if (device->usb_device->driver != NU_NULL)
    {
        /* Disable this driver. */
        status = NU_USB_DRVR_Disconnect (usb_device->driver,
                                usb_device->stack, usb_device);

        usb_device->driver = NU_NULL;
        device->active_config = NU_NULL;

        return(status);
    }

    /* Check if there are any interface   level drivers for any of the
     * interfaces of this device.
     */

    /* If the device is not configured yet, ignore it. */

    if (device->active_config == NU_NULL)
    {
        return(status);
    }
    for (j = 0; j < device->active_config->desc->bNumInterfaces; j++)
    {
        if (device->active_config->intf[j].driver != NU_NULL)
        {
            /* Disable this driver. */
            status = NU_USB_DRVR_Disconnect (
                                     device->active_config->intf[j].driver,
                                     usb_device->stack, usb_device);
            device->active_config->intf[j].driver = NU_NULL;
        }
    }

    device->active_config = NU_NULL;

    NU_UNUSED_PARAM(status);

    return(NU_SUCCESS);
}

/**************************************************************************
*
* FUNCTION
*
*       usbf_get_device
*
* DESCRIPTION
*
*       This function returns the device (pointer) associated with a given
*       hardware pointer.
*
*       It looks through the device list maintained in the stack control
*       block and tries for a match for the hardware pointer inside the
*       device structure. Once a match is found, then the device pointer
*       and its index are returned.
*
* INPUTS
*
*       cb                                  Stack control block.
*       fc                                  Hardware pointer to find a
*                                           device for.
*       index                               index reference where the found
*                                           index is to be stored.
*
* OUTPUTS
*
*       NU_NULL                             If not found.
*
**************************************************************************/
NU_USBF_DEVICE *    usbf_get_device (NU_USB_STACK *cb,
                                     NU_USBF_HW   *fc,
                                     INT          *index)
{
    INT             i = 0;
    NU_USBF_STACK  *stack = NU_NULL;

    if ( cb == NU_NULL || fc == NU_NULL || index == NU_NULL)
    {
        return NU_NULL;
    }

    stack  = (NU_USBF_STACK *) cb;

    /* In any entry of the FC list, if the "fc" pointer matches the
     * supplied then it is returned.
     */
    for (i = 0; i < stack->num_devices; i++)
    {
        if ((NU_USB_HW *) fc == stack->device_list[i].usb_device->hw)
        {
            *index = i;
            return (&stack->device_list[i]);
        }
    }

    return (NU_NULL);
}

/**************************************************************************
*
* FUNCTION
*
*       usbf_validate_setup
*
* DESCRIPTION
*
*       This function validates the incoming setup against the device
*       state. Finer validation would be done by the individual processing
*       functions.
*
* INPUTS
*
*       device                              Device on which the setup
*                                           packet is received.
*       setup                               The 8 byte setup packet.
*
* OUTPUTS
*
*       NU_SUCCESS                          If the incoming request is
*                                           valid.
*       NU_USB_NOT_SUPPORTED                Otherwise.
*
**************************************************************************/
STATUS  usbf_validate_setup (NU_USBF_DEVICE   *device,
                             NU_USB_SETUP_PKT *setup)
{
    UINT8   bRequest ;
    UINT16   wValue   ;
    
    /* Error checking.   */
    if ((device  == NU_NULL) || (setup == NU_NULL))
    {
        return  NU_USB_INVLD_ARG;
    } 
 
    bRequest = setup->bRequest;
    wValue = setup->wValue;
    
    if (bRequest == USB_SET_FEATURE)
    {
        if ((wValue >= 3) && (wValue <= 5))    /* OTG features */
        {
            return (NU_SUCCESS);
        }
    }

    switch (device->state)
    {
        case USB_STATE_DEFAULT:

            /* Only SET ADDRESS and GET DESCRIPTOR are supported.   */
            if ((bRequest == USB_SET_ADDRESS)
                || (bRequest == USB_GET_DESCRIPTOR))
            {
                return (NU_SUCCESS);
            }
            return (NU_USB_NOT_SUPPORTED);

        case USB_STATE_ADDRESSED:

            /* G(S)ET INTERFACE and SYNC FRAME are not supported  */
            if ((bRequest == USB_GET_INTERFACE) ||
                (bRequest == USB_SET_INTERFACE) ||
                (bRequest == USB_SYNC_FRAME))
            {
                return (NU_USB_NOT_SUPPORTED);
            }
            return (NU_SUCCESS);

        case USB_STATE_CONFIGURED:

            /* Only SET ADDRESS is not supported.    */
            if (bRequest == USB_SET_ADDRESS)
            {
                return (NU_USB_NOT_SUPPORTED);
            }
            return (NU_SUCCESS);

        default:
            break;
    }

    return (NU_USB_NOT_SUPPORTED);
}

/**************************************************************************
*
* FUNCTION
*
*       usbf_ep0_transfer_complete
*
* DESCRIPTION
*
*       The following function is the transfer complete processing
*       function for all control transfers owned by the stack i.e.,
*       the standard control requests.
*
*       This function doesn't do any thing.
*
* INPUTS
*
*       pipe                                The control pipe reference.
*       irp                                 Reference to transfer that has
*                                           just been completed.
*
* OUTPUTS
*
*       NU_SUCCESS                          Always.
*
**************************************************************************/
VOID    usbf_ep0_transfer_complete (NU_USB_PIPE *pipe,
                                    NU_USB_IRP  *irp)
{
    /* Removal of unused parameter warning. */
    NU_UNUSED_PARAM (pipe);

    /* Removal of unused parameter warning. */
    NU_UNUSED_PARAM (irp);

    /* As this function returns after a successful data phase, and status
     * phase automatically handled in the underlying layer, there is
     * nothing much this needs to do..
     */

    return;
}

/**************************************************************************
*
* FUNCTION
*
*       usbf_validate_endpoint
*
* DESCRIPTION
*
*       The following function validates if the given endpoint is present
*       in the current configuration. If the endpoint is valid, then the
*       driver associated with the containing interface, if any, is
*       returned.
*
* INPUTS
*
*       device                              Device on which the endpoint
*                                           is to be located.
*       endp                                Endpoint to be located.
*       driver                              Place-holder where the driver
*                                           pointer is to be stored.
*
* OUTPUTS
*
*       NU_SUCCESS                          If the endpoint could be
*                                           located successfully.
*       NU_USB_INVLD_ARG                    If the endpoint could not be
*                                           located
*
**************************************************************************/
STATUS  usbf_validate_endpoint (NU_USBF_DEVICE  *device,
                                UINT8            endp,
                                NU_USB_DRVR    **driver)
{
    NU_USB_CFG         *cfg = NU_NULL;
    NU_USB_ALT_SETTG   *alt_settg;

    INT                 i = 0;
    INT                 j = 0;
    
    /* Error checking.   */
    if ((device  == NU_NULL) || (device->active_config == NU_NULL)) 
    
    {
        return  NU_USB_INVLD_ARG;
    } 
 
    cfg = device->active_config;
    
    for (i = 0; i < cfg->desc->bNumInterfaces; i++)
    {
        alt_settg = cfg->intf[i].current;

        for (j = 0; j < alt_settg->desc->bNumEndpoints; j++)
        {
            if (endp == alt_settg->endp[j].desc->bEndpointAddress)
            {
                if (driver != NU_NULL)
                {
                    if(device->usb_device->driver != NU_NULL)
                        *driver = device->usb_device->driver;
                    else
                        *driver = cfg->intf[i].driver;
                }
                return (NU_SUCCESS);
            }
        }
    }

    return (NU_USB_INVLD_ARG);
}

/**************************************************************************
*
* FUNCTION
*
*       usbf_validate_interface
*
* DESCRIPTION
*
*       The following function validates if the given interface is present
*       in the current configuration. If the interface is valid, then the
*       driver associated with the interface, if any, is returned.
*
* INPUTS
*
*       device                              Device on which the interface
*                                           is to be located.
*       interface                           interface to be located.
*       driver                              Place-holder where the driver
*                                           pointer is to be stored.
*
* OUTPUTS
*
*       NU_SUCCESS                          If the interface could be
*                                           located successfully.
*       NU_USB_INVLD_ARG                    If the interface could not be
*                                           located.
*
**************************************************************************/
STATUS  usbf_validate_interface (NU_USBF_DEVICE  *device,
                                 UINT8            interface,
                                 NU_USB_DRVR    **driver)
{
    NU_USB_CFG *cfg = NU_NULL;
    STATUS status = NU_SUCCESS;

    /* Error checking.   */
    if (device != NU_NULL)
    {
        cfg = device->active_config;
        if(interface >= cfg->desc->bNumInterfaces)
            status = NU_USB_INVLD_ARG;
    } 
    else
    {
        status = NU_USB_INVLD_ARG;
    } 
 
    if (status == NU_SUCCESS && driver != NU_NULL)
    {
        *driver = cfg->intf[interface].driver;
    }

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       usbf_process_class_specific
*
* DESCRIPTION
*
*       This function processes the class specific requests that have come
*       in.
*
* INPUTS
*
*       device                              device on which the setup
*                                           packet is received.
*       setup                               The 8 byte setup packet.
*
* OUTPUTS
*
*       NU_SUCCESS                          If the request is valid and
*                                           processed successfully by the
*                                           corresponding driver.
*       Error                               Status returned by the caller
*                                           otherwise.
*
**************************************************************************/
STATUS  usbf_process_class_specific (NU_USBF_DEVICE   *device,
                                     NU_USB_SETUP_PKT *setup)
{
    UINT32            recipient ;
    NU_USB_DRVR       *driver = NU_NULL;
    UINT8             temp ;
    STATUS            status = NU_USB_INVLD_ARG;
    NU_USB_STACK      *stack = NU_NULL;
    NU_USB_SETUP_PKT    set_config = {0x00,0x09,0x0001,0x0000,0x0000 };
    
     /* Error checking.   */
    if (device == NU_NULL || setup == NU_NULL)
    {
        return  NU_USB_INVLD_ARG;
    } 
     
    recipient = USBF_GET_RECIPIENT(setup);
    temp = (UINT8) (setup->wIndex);
    stack = (device->usb_device->stack);
    
    /* Check the recipient. It may be endpoint or an interface,
     * Validate the recipient and obtain the driver associated
     */
    switch (recipient)
    {
        case USB_RECIPIENT_DEVICE:
        {
            /* Get_Functional Descriptor Request. */
            if(setup->bRequest == MTP_GET_FUNCTIONAL_DESC)
            {
            /* Microsoft MTP Driver calls this descriptor before set config
             * so we have to set a dummy one from ourselves just to
             * attach a user driver to the device. This is due to the
             * fact that this Get_Functional_Descriptor request is
             * answered by the user driver.
             */
                status = usbf_set_configuration(device, 
                                          (NU_USB_SETUP_PKT*)&set_config);
            }

            if (status == NU_SUCCESS)
            {
                status = usbf_validate_interface (device, 0x00, &driver);
            }
            break;
        }
        case USB_RECIPIENT_INTERFACE:
            status = usbf_validate_interface (device, temp, &driver);
            break;

        case USB_RECIPIENT_ENDPOINT:
            status = usbf_validate_endpoint (device, temp, &driver);
            break;

        default:
            status = NU_USB_INVLD_ARG;
            break;
    }

    /* If recipient could be validated and driver is found then hand over
     * the request to the driver for processing.
     */
    if ((status == NU_SUCCESS) && (driver != NU_NULL))
    {
        status = NU_USBF_DRVR_New_Setup (driver, stack,
                                         device->usb_device, setup);
    }

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       usbf_request_error
*
* DESCRIPTION
*
*       This function is the default handler for un-supported requests
*       It returns error code, resulting in a stall on the control
*       endpoint.
*
* INPUTS
*
*       device                              Device on which the setup
*                                           packet is received.
*       setup                               The 8 byte setup packet.
*
* OUTPUTS
*
*       NU_USB_NOT_SUPPORTED                Always.
*
**************************************************************************/
STATUS  usbf_request_error (NU_USBF_DEVICE   *device,
                            NU_USB_SETUP_PKT *setup)
{
    /* Removal of unused parameter warning. */
    NU_UNUSED_PARAM (device);

    /* Removal of unused parameter warning. */
    NU_UNUSED_PARAM (setup);

    return (NU_USB_NOT_SUPPORTED);
}

/**************************************************************************
*
* FUNCTION
*
*       usbf_get_status
*
* DESCRIPTION
*
*       GET STATUS processing function.
*
* INPUTS
*
*       device                              Device on which the setup
*                                           packet is received.
*       setup                               The 8 byte setup packet.
*
* OUTPUTS
*
*       Status
*
**************************************************************************/
STATUS  usbf_get_status (NU_USBF_DEVICE   *device,
                         NU_USB_SETUP_PKT *setup)
{
    UINT32  recipient ;
    STATUS  status = NU_USB_INVLD_ARG;
    UINT16  value = 0;
    
    /* Error checking.   */
    if ((device  == NU_NULL) || (setup == NU_NULL))
    {
        return  NU_USB_INVLD_ARG;
    } 
    
    recipient = USBF_GET_RECIPIENT(setup);
    
    /* Validate the request. 
     * According USB 2.0 Specs Section 9.4.5
     * If device is in addressed state and then 
     * an interface or an endpoint other than endpoint zero is specified,
     * then the device responds with a Request Error.
     */
    if (device->state != USB_STATE_CONFIGURED)
    {
        if (recipient == USB_RECIPIENT_INTERFACE)
        {
            return (NU_USB_INVLD_ARG);
        }
        if ((recipient == USB_RECIPIENT_ENDPOINT) && (setup->wIndex != 0))
        {
            return (NU_USB_INVLD_ARG);
        }
    }
    /* According USB 2.0 Specs Section 9.4.5
     * "For "Get Status" request
     * "If wValue != 0 or wLength != 2, 
     * or if wIndex is non-zero for a device status request, 
     * then the behavior of the device is not specified." 
     */
    if((setup->wValue != 0)
        || (setup->wLength != 2))
    {
        return  NU_USB_INVLD_ARG;
    }
    
    /* Process the request */
    USBF_SET_LENGTH (device->ctrl_irp, 2UL);
    USBF_SET_BUFFER (device->ctrl_irp, device->buffer);

    switch (recipient)
    {
        case USB_RECIPIENT_DEVICE:
            status = NU_USBF_HW_Get_Status (
                            (NU_USBF_HW *) (device->usb_device->hw), &value);
            break;

        case USB_RECIPIENT_INTERFACE:
            status = usbf_validate_interface (device,
                                 (UINT8) setup->wIndex, NU_NULL);
            break;

        case USB_RECIPIENT_ENDPOINT:
            /* for non-zero endpoint, check if it is valid */
            if((UINT8) setup->wIndex > 0)
            {
                status = usbf_validate_endpoint (device, 
                                           (UINT8) setup->wIndex, NU_NULL);

                if (status != NU_SUCCESS)
                {
                    break;
                }
            }

            status = NU_USBF_HW_Get_Endpoint_Status (
                                 (NU_USBF_HW *) (device->usb_device->hw),
                                 (UINT8) setup->wIndex, &value);
            break;

        default:
            break;
    }

    if (status == NU_SUCCESS)
    {
        device->buffer[0] =(UINT8) value;
        device->buffer[1] =(UINT8) (value >> 8);
    }

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       usbf_feature
*
* DESCRIPTION
*
*       SET FEATURE and CLEAR FEATURE processing function.
*
* INPUTS
*
*       device                              Device on which the setup
*                                           packet is received.
*       setup                               The 8 byte setup packet.
*
* OUTPUTS
*
*       Status
*
**************************************************************************/
STATUS  usbf_feature (NU_USBF_DEVICE   *device,
                      NU_USB_SETUP_PKT *setup)
{
    UINT32  recipient;
    STATUS  status = NU_USB_INVLD_ARG;
    UINT16   wIndex ;
    NU_USBF_HW *hw = NU_NULL;
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
    UINT8   suspend_options;
    BOOLEAN low_power_suspended_state;
    BOOLEAN function_remote_wake_enabled;
    NU_USB_INTF * intf;
#endif
    
    /* Error checking.   */
    if ((device  == NU_NULL) || (setup == NU_NULL))
    {
        return  NU_USB_INVLD_ARG;
    }
     
    recipient = USBF_GET_RECIPIENT(setup);
    status      = NU_USB_INVLD_ARG;
    wIndex    = setup->wIndex;
    hw        = (NU_USBF_HW *)(device->usb_device->hw);

    /* Validate the request 
     * According USB 2.0 Specs Section 9.4.1 & 9.4.9
     * This request is valid when the device is in the Address state; 
     * references to interfaces or to endpoints other than endpoint zero
     * shall cause the device to respond with a Request Error.
     */
    if (device->state != USB_STATE_CONFIGURED)
    {
        if (recipient == USB_RECIPIENT_INTERFACE)
        {
            return (NU_USB_INVLD_ARG);
        }
        
        if ((recipient == USB_RECIPIENT_ENDPOINT) && (wIndex != 0))
        {
            return (NU_USB_INVLD_ARG);
        }
    }

    /* Process the request.      */
    USBF_SET_LENGTH (device->ctrl_irp, 0UL);

    switch (recipient)
    {
        case USB_RECIPIENT_DEVICE:
        {
            /* If recipient is device then Set/Clear feature request is 
             * only valid when device is in configured state.
             */
            switch(device->state)
            {
                case USB_STATE_CONFIGURED:
                    break;
                case USB_STATE_POWERED:
                case USB_STATE_DEFAULT:
                case USB_STATE_ADDRESSED:
                case USB_STATE_SUSPENDED:                    
                default:
                    return ( NU_USB_INVLD_STATE );
            }

            /* USB 2.0/3.0 specs section 9.4.1 & 9.4.9
             * In a Set/Clear feature request, if recipient is device then
             * wIndex and wLength must be set to zero.
             */
            if ( ((setup->wIndex & 0x00FF) != 0 ) || (setup->wLength != 0 ) )
            {
                return ( NU_USB_INVLD_REQUEST );
            }
            
            if (setup->bRequest == USB_SET_FEATURE)
            {
                switch(setup->wValue)
                {
                    case USB_FEATURE_TEST_MODE:
                    {
                        UINT8 *tm_ptr;             /* test mode pointer */
                        tm_ptr = (UINT8 *)&setup->wIndex;
                        hw->test_mode = tm_ptr[1];
#if (USB_TEST_MODE_SUPPORT == NU_TRUE)
                        NU_USBF_Test_Execute((NU_USB_HW*)hw, hw->test_mode);
#endif
                        return (NU_SUCCESS);
                    }
                    case USB_FEATURE_B_HNP_ENABLE:
                    {
                        device->usb_device->otg_status |= 0x20U;
                        NU_USBF_HW_Start_HNP(hw, 0x01);
                        return (NU_SUCCESS);
                    }
                    case USB_FEATURE_A_HNP_SUPPORT:
                    {
                        device->usb_device->otg_status  |= 0x40U;
                        return (NU_SUCCESS);
                    }
                    case USB_FEATURE_A_ALT_HNP_SUPPORT:
                    {
                        device->usb_device->otg_status |= 0x80U;
                        return (NU_SUCCESS);
                    }
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
                    case USB_FEATURE_U1_ENABLE:
                    {
                        status = NU_USBF_HW_Set_Ux_Enable(hw, USB_LINK_STATE_U1, NU_TRUE);
                        return (status);
                    }    
                    case USB_FEATURE_U2_ENABLE:
                    {
                        status = NU_USBF_HW_Set_Ux_Enable(hw, USB_LINK_STATE_U2, NU_TRUE);
                        return (status);
                    }
                    case USB_FEATURE_LTM_ENABLE:
                    {
                        status = NU_USBF_HW_Set_LTM_Enable(hw, NU_TRUE);
                        return (status);
                    }
#endif
                    default:
                        break;
                }
            }
            else if (setup->bRequest == USB_CLEAR_FEATURE)
            {
                switch(setup->wValue)
                {
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
                    case USB_FEATURE_U1_ENABLE:
                    {
                        status = NU_USBF_HW_Set_Ux_Enable(hw, USB_LINK_STATE_U1, NU_FALSE);
                        return (status);
                    }    
                    case USB_FEATURE_U2_ENABLE:
                    {
                        status = NU_USBF_HW_Set_Ux_Enable(hw, USB_LINK_STATE_U2, NU_FALSE);
                        return (status);
                    }
                    case USB_FEATURE_LTM_ENABLE:
                    {
                        status = NU_USBF_HW_Set_LTM_Enable(hw, NU_FALSE);
                        return (status);
                    }
#endif
                    case USB_FEATURE_TEST_MODE:
                    case USB_FEATURE_B_HNP_ENABLE:
                    case USB_FEATURE_A_HNP_SUPPORT:
                    case USB_FEATURE_A_ALT_HNP_SUPPORT:
                    default:
                        break;
                }
            }
            
            /* If recipient is device then only valid feature selectors are
             * Test Mode
             * B_HNP_Enable
             * A_HNP_Enable
             * A_ALT_HNP_Enable
             * U1_Enable
             * U2_Enable
             * LTM_Enable
             */
            return (NU_USB_INVLD_REQUEST);
        }
        case USB_RECIPIENT_ENDPOINT:
        {
            switch(device->state)
            {
                /* This is a valid request when device is in configured 
                 * state.
                 */
                case USB_STATE_CONFIGURED:
                    break;
                /* If device is in addressed state then only valid endpoint
                 * is default control endpoint.
                 */
                case USB_STATE_ADDRESSED:
                    if (setup->wIndex != 0)
                    {
                        return (NU_USB_INVLD_REQUEST);
                    }
                    break;
                case USB_STATE_POWERED:
                case USB_STATE_DEFAULT: 
                case USB_STATE_SUSPENDED:                    
                default:
                    return ( NU_USB_INVLD_STATE );
            }

            status = usbf_validate_endpoint (device, wIndex, NU_NULL);
            if (status != NU_SUCCESS)
            {
                return (status);
            }
            
            if (setup->bRequest == USB_SET_FEATURE)
            {
                switch(setup->wValue)
                {
                    case USB_FEATURE_ENDPOINT_HALT:
                    {
                        status = NU_USBF_HW_Stall_Endpoint (hw, wIndex);
                        return (status);
                    }
                    default:
                        break;
                }
            }
            else if (setup->bRequest == USB_CLEAR_FEATURE)
            {
                switch(setup->wValue)
                {
                    case USB_FEATURE_ENDPOINT_HALT:
                    {
                        status = NU_USBF_HW_Unstall_Endpoint (hw, wIndex);
                        if(status  == NU_SUCCESS)
                        {  
                            status = NU_USBF_STACK_Notify (device->usb_device->stack, hw, 
                                                  USBF_EVENT_CLEAR_HALTENDPOINT);
                        }
                        return (status);
                    }
                    default:
                        break;
                }
            }
            
            /* If recipient is endpoint then only valid feature selectors are
             * Endpoint Halt
             */
            return (NU_USB_INVLD_REQUEST);
        }
        case USB_RECIPIENT_INTERFACE:
        {
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
            /* The upper byte of wIndex contains the Suspend Options in 
             * this case.
             */
            suspend_options = (setup->wIndex >> 8 );

            /* Set the required variables using the selected bits from
             * the suspend options.
             */
            low_power_suspended_state =
              (FUNCTION_SUSPEND_BIT_POS & suspend_options)?NU_TRUE:
                                                                 NU_FALSE;
            function_remote_wake_enabled =
              (FUNCTION_REMOTE_WAKEUP_BIT_POS & suspend_options)?NU_TRUE:
                                                                 NU_FALSE;
            
            /* The lower byte of wIndex is set to the first interface
             * that is part of the function.
             */
            status = usbf_validate_interface (device, wIndex, NU_NULL);

            if (status == NU_SUCCESS)
            {
                if (setup->bRequest == USB_SET_FEATURE)
                {
                    /* Obtain the required interface. */
                    status = NU_USB_CFG_Get_Intf(device->active_config,
                                                        wIndex, &intf);
                    if ( status == NU_SUCCESS )
                    {
                        status = NU_USB_STACK_Function_Suspend(
                                        device->usb_device->stack,
                                        intf,
                                        low_power_suspended_state,
                                        function_remote_wake_enabled);
                        return ( status );
                    }
                }
            }
#endif
            return ( NU_USB_INVLD_REQUEST );
        }
        default:
            break;
    }

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       usbf_set_address
*
* DESCRIPTION
*
*       SET ADDRESS request processing.
*
* INPUTS
*
*       device                              Device on which the setup
*                                           packet is received.
*       setup                               The 8 byte setup packet.
*
* OUTPUTS
*
*       Status
*
**************************************************************************/
STATUS  usbf_set_address (NU_USBF_DEVICE   *device,
                          NU_USB_SETUP_PKT *setup)
{
    UINT8   address ;
    STATUS  status;
    UINT32  capability = 0UL;
 
     /* Error checking.   */
    if ((device  == NU_NULL) || (setup == NU_NULL))
    {
        return  NU_USB_INVLD_ARG;
    }
    
    address =(UINT8)setup->wValue;
    USBF_SET_LENGTH (device->ctrl_irp, 0UL);/* No data phase.            */

    if (address > USB_MAX_ADDRESS)
    {
        return (NU_USB_INVLD_ARG);
    }
    
    /* According USB 2.0 Specs Section 9.4.6
     * "If wIndex or wLength are non-zero then 
     * behaviour of device is unspecified." */
     
    if((setup->wIndex != 0)
       ||(setup->wLength != 0))
    {
        return (NU_USB_INVLD_ARG);
    }

    if (address == 0)
    {
        status = usbf_disable_device (device);
    }
    else
    {
        /* Check the FC capabilities. If it supported 'automatic' response
         * to SET_CONFIGURATION then, relevant state changes
         * are to be done here itself.  otherwise the device
         * goes to the addressed state.
         */
        status = NU_USBF_HW_Get_Capability (
                                   (NU_USBF_HW *)(device->usb_device->hw),
                                   &capability);

        if (status == NU_SUCCESS)
        {
            /* Handles SET_CFG.  */
            if (capability & (0x01 << USB_SET_CONFIGURATION))
            {
                /* If the device supports automatic response to
                 * SET_CONFIGURATION, then, we move the device to configured
                 * state.
                 */
                device->state = USB_STATE_CONFIGURED;
            }
            else
            {
                device->state = USB_STATE_ADDRESSED;
            }
        }
    }

    if (status == NU_SUCCESS)
    {
        status =
          NU_USBF_HW_Set_Address ((NU_USBF_HW *) (device->usb_device->hw),
                                  address);
    }
    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       usbf_get_descriptor
*
* DESCRIPTION
*
*       GET DESCRIPTOR request processing.
*
* INPUTS
*
*       device                              Device on which the setup
*                                           packet is received.
*       setup                               The 8 byte setup packet.
*
* OUTPUTS
*
*       Status
*
**************************************************************************/
STATUS  usbf_get_descriptor (NU_USBF_DEVICE   *device,
                             NU_USB_SETUP_PKT *setup)
{
    UINT8               desc_type ;
    UINT8               desc_index ;
    UINT16              wLangId ;
    UINT16              actual_length ;
    UINT16              requested_length ;
    NU_USB_DEVICE_DESC *dev;
    UINT8              *cfg;
    UINT16              size;
    INT                 i;
    NU_USB_DEVICE      *usb_device ;
    
     /* Error checking.   */
    if ((device  == NU_NULL) || (setup == NU_NULL))
    {
        return  NU_USB_INVLD_ARG;
    }

    desc_type = (UINT8) (setup->wValue >> 8U);
    desc_index = (UINT8) (setup->wValue); 
    wLangId = setup->wIndex;
    requested_length = setup->wLength;
    usb_device = device->usb_device;

    switch (desc_type)
    {
        case USB_DT_DEVICE:

            /* copy Device_Descriptor to buffer */
            memcpy(device->buffer, &usb_device->device_descriptor, 18);

            /* Using size as temporary variable, convert the 16 bit fields. */
            size = usb_device->device_descriptor.bcdUSB;
            device->buffer[2] = size & 0x00ff;
            device->buffer[3] = (size >> 8) & 0x00ff;

            /* if operating at other speed, 
               get some fields from Device_Qualifier */
            if (device->other_speed)
            {
                /* bcdUSB, bDevicecClass, bDeviceSubClass,
                   bDeviceProtocol, bMaxPacketSize0 */
                memcpy(device->buffer + 2, 
                      (UINT8 *)(&usb_device->device_qualifier) + 2, 6);

                /* bNumConfigurations */
                device->buffer[17] =
                    usb_device->device_qualifier.bNumConfigurations;
            }

            size = usb_device->device_descriptor.idVendor;
            device->buffer[8] = size & 0x00ff;
            device->buffer[9] = (size >> 8) & 0x00ff;

            size = usb_device->device_descriptor.idProduct;
            device->buffer[10] = size & 0x00ff;
            device->buffer[11] = (size >> 8) & 0x00ff;

            size = usb_device->device_descriptor.bcdDevice;
            device->buffer[12] = size & 0x00ff;
            device->buffer[13] = (size >> 8) & 0x00ff;

             /* Device Descriptor request.    */
            USBF_SET_BUFFER (device->ctrl_irp, (UINT32 *) (device->buffer));

            /* size of Device Descriptor is always 18 bytes */
            actual_length = 18U;

            break;

        case USB_DT_CONFIG:

            /* Config Descriptor request.    */
            dev = &usb_device->device_descriptor;

            if (desc_index >= dev->bNumConfigurations)
            {
                return (NU_USB_INVLD_ARG);
            }

            cfg = usb_device->raw_descriptors[desc_index][usb_device->speed];
            if(cfg != NU_NULL)
            {
                actual_length = (UINT16) (cfg[2]);
                actual_length |= (((UINT16) (cfg[3])) << 8);
            }
            else
            {
                return (NU_USB_INVLD_DESC);
            }


            cfg[1] = USB_DT_CONFIG;

            USBF_SET_BUFFER (device->ctrl_irp, (UINT32 *) (cfg));

            break;

        case USB_DT_STRING:

            /* String Descriptor request.    */
            for (i = 0; i < usb_device->num_string_descriptors; i++)
            {
                if ((desc_index == usb_device->string_descriptors[i]->str_index)
                      && ((desc_index == 0) ||
                    (wLangId == usb_device->string_descriptors[i]->wLangId)))
                {
                    break;
                }
            }

            if (i == usb_device->num_string_descriptors)
            {
                return (NU_USB_INVLD_ARG);
            }

            USBF_SET_BUFFER (device->ctrl_irp,
               (UINT32 *) (&usb_device->string_descriptors[i]->string[0]));

            actual_length = usb_device->string_descriptors[i]->string[0];

            break;

        case USB_DT_DEVICE_QUALIFIER:

            /* full-speed only device should respond with a request error */
            if(usb_device->device_qualifier.bLength == 0)
            {
                return (NU_USB_REQUEST_ERROR);
            }

            /* copy Device_Qualifier to buffer */
            memcpy(device->buffer, &usb_device->device_qualifier, 10);

            /* if operating at other speed, 
               update some fields with Device_Descriptor */
            if (device->other_speed)
            {
                /* bcdUSB, bDevicecClass, bDeviceSubClass,
                   bDeviceProtocol, bMaxPacketSize0 */
                memcpy(device->buffer + 2, 
                      (UINT8 *)(&usb_device->device_descriptor) + 2, 6);

                /* bNumConfigurations */
                device->buffer[8] = 
                    usb_device->device_descriptor.bNumConfigurations;
            }

            /* Using size as temporary variable, convert the 16 bit fields. */
            size = usb_device->device_qualifier.bcdUSB;
            device->buffer[2] = size & 0x00ff;
            device->buffer[3] = (size >> 8) & 0x00ff;

            /* Device qualifier request. */
            USBF_SET_BUFFER (device->ctrl_irp, (UINT32 *)(device->buffer));

            /* size of Device Qualifier is always 10 bytes */
            actual_length = 10;

            break;

        case USB_DT_OTHER_SPEED_CONFIG:

            /* if operating at the default speed, advance to
             * Other_Speed Configuration descriptor. 
             */
            if(device->other_speed == 0)  /* default speed */
            {
                /* get default speed configuration */
                cfg = usb_device->raw_descriptors[desc_index][USB_SPEED_FULL];
                if(cfg != NU_NULL)
                {
                    actual_length = (UINT16) (cfg[2]);
                    actual_length |= (((UINT16) (cfg[3])) << 8);
                }
                else
                {
                    return ( NU_USB_INVLD_DESC );
                }
            }
            else
            {
                return (NU_USB_INVLD_DESC);
            } 

            cfg[1] = USB_DT_OTHER_SPEED_CONFIG;

            USBF_SET_BUFFER (device->ctrl_irp, (UINT32 *) (cfg));
            break;
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
        case USB_DT_BOS:
            
            /* If (BOS descriptor is null)*/
            if (usb_device->raw_bos_descriptors == NU_NULL)
            {
                return (NU_USB_INVLD_DESC);
            }
            else
            {
                /* Copy BOS descriptor in output buffer. */
                USBF_SET_BUFFER (device->ctrl_irp,
                    (UINT32 *) (usb_device->raw_bos_descriptors));
                    
                /* Modify actual_length to reflect total length of
                   BOS descriptor. It is the UINT16 length indicated in [2]
                   and [3] of the raw BOS descriptor buffer. */
                actual_length =
                    (UINT16)(usb_device->raw_bos_descriptors[2]);
                actual_length |=
                    (((UINT16)(usb_device->raw_bos_descriptors[3])) << 8);
            }
            break;
#endif
        default:

            return (NU_USB_INVLD_ARG);
    }

    if (requested_length <= actual_length)
    {
        actual_length = requested_length;
    }
    else
    {
        UINT8  mps = 0;
        
        if (device->other_speed)
            mps = usb_device->device_qualifier.bMaxPacketSize0;
        else
            mps = usb_device->device_descriptor.bMaxPacketSize0;

        if (actual_length % mps == 0)
        {
            device->ctrl_irp.flags = USB_ZERO_PACKET;
        }
    }

    USBF_SET_LENGTH (device->ctrl_irp, actual_length);

    return (NU_SUCCESS);
}

/**************************************************************************
*
* FUNCTION
*
*       usbf_set_descriptor
*
* DESCRIPTION
*
*       SET DESCRIPTOR request processing. This is an optional request and
*       is not supported currently.
*
* INPUTS
*
*       device                              Device on which the setup
*                                           packet is received.
*       setup                               The 8 byte setup packet.
*
*
* OUTPUTS
*
*       NU_USB_NOT_SUPPORTED                Always.
*
**************************************************************************/
STATUS  usbf_set_descriptor (NU_USBF_DEVICE   *device,
                             NU_USB_SETUP_PKT *setup)
{
    /* Removal of unused parameter warning. */
    NU_UNUSED_PARAM (device);

    /* Removal of unused parameter warning. */
    NU_UNUSED_PARAM (setup);

    /* Currently, this request is not supported.    */
    return (NU_USB_NOT_SUPPORTED);
}

/**************************************************************************
*
* FUNCTION
*
*       usbf_get_interface
*
* DESCRIPTION
*
*       GET INTERFACE request processing.
*
* INPUTS
*
*       device                              Device on which the setup
*                                           packet is received.
*       setup                               The 8 byte setup packet.
*
* OUTPUTS
*
*       NU_SUCCESS                          If the interface could be found
*       NU_USB_INVLD_ARG                    For invalid interfaces.
*
**************************************************************************/
STATUS  usbf_get_interface (NU_USBF_DEVICE   *device,
                            NU_USB_SETUP_PKT *setup)
{
    NU_USB_CFG *cfg = NU_NULL ;
    UINT8  interface ;
     
    /* Error checking.   */
    if (device == NU_NULL || setup == NU_NULL)
    {
        return  NU_USB_INVLD_ARG;
    }
    
    /* Validate the request 
     * According USB 2.0 Specs Section 9.4.4
     * This is a valid request when the device is in the 
     * Configured state.
     * Also If wValue != 0 or wLength != 1, 
     * then the device behavior is not specified.
     */
    if(device->state != USB_STATE_CONFIGURED)
    {
        return  NU_USB_INVLD_ARG;
    }
    
    if((setup->wValue != 0)
        || (setup->wLength != 1))
    {
        return  NU_USB_INVLD_ARG;
    }

    cfg = device->active_config;
    interface = (UINT8) setup->wIndex;

    USBF_SET_LENGTH (device->ctrl_irp, 1UL);
    USBF_SET_BUFFER (device->ctrl_irp, (UINT32 *)device->buffer);

    /* Validate the interface.  */
    if (interface >= cfg->desc->bNumInterfaces)
    {
        return (NU_USB_INVLD_ARG);
    }
    device->buffer[0] =
            cfg->intf[interface].current->desc->bAlternateSetting;

    return (NU_SUCCESS);
}

/**************************************************************************
*
* FUNCTION
*
*       usbf_set_interface
*
* DESCRIPTION
*
*       SET INTERFACE request processing.
*
* INPUTS
*
*       device                              Device on which the setup
*                                           packet is received.
*       setup                               The 8 byte setup packet.
*
* OUTPUTS
*
*       NU_SUCCESS                          If everything goes fine.
*       NU_USB_INVLD_ARG                    If either the interface number
*                                           / alt_setting number is wrong.
*
**************************************************************************/
STATUS  usbf_set_interface (NU_USBF_DEVICE   *device,
                            NU_USB_SETUP_PKT *setup)
{
    NU_USB_CFG     *cfg;
    INT             i = 0;
    INT             alt_settg_index;
    UINT8           alt_settg;
    UINT8           interface;
    NU_USB_INTF    *intf_info;
    NU_USB_ALT_SETTG *current_setting;
    STATUS          status = NU_SUCCESS;
    STATUS          int_status = NU_SUCCESS;
    BOOLEAN         roll_back = NU_FALSE;

    NU_USB_DRVR    *driver;
    NU_USB_STACK   *stack;
    UINT8           endpoint;

    /* Error checks. */
    if ((device  == NU_NULL) || (setup == NU_NULL) ||
        (device->usb_device->stack == NU_NULL))
    {
        return NU_USB_INVLD_ARG;
    }

    cfg = device->active_config;
    alt_settg = (UINT8) setup->wValue;
    interface = (UINT8) setup->wIndex;
    stack = device->usb_device->stack;
    USBF_SET_LENGTH (device->ctrl_irp, 0UL);
    
    /* Validate the interface.   */
    if (interface >= cfg->desc->bNumInterfaces)
    {
        return (NU_USB_INVLD_ARG);
    }
    
    intf_info = &cfg->intf[interface];
    current_setting = intf_info->current;
    
    /* Validate the alternate setting.   */
    for (i = 0; i < NU_USB_MAX_ALT_SETTINGS; i++)
    {
        if (alt_settg == intf_info->alt_settg[i].desc->bAlternateSetting)
        {
            break;
        }
    }

    if (i == NU_USB_MAX_ALT_SETTINGS)
    {
        return (NU_USB_INVLD_ARG);
    }

    alt_settg_index = i;

    if (intf_info->current == &(intf_info->alt_settg[i]))
    {
        return (NU_SUCCESS);
    }

    /* Disable all the endpoints in old alternate setting.   */
    for (i = 0; i < current_setting->desc->bNumEndpoints; i++)
    {
        endpoint = current_setting->endp[i].desc->bEndpointAddress;

        status |= NU_USB_HW_Close_Pipe (device->usb_device->hw, 0, endpoint);
    }
    
    if (status == NU_SUCCESS)
    {
        /* Change the alternate setting. */
        intf_info->current = &(intf_info->alt_settg[alt_settg_index]);

        /* Enable all the endpoints in new alternate setting.    */
        for (i = 0; i < intf_info->current->desc->bNumEndpoints; i++)
        {
            UINT8   type = intf_info->current->endp[i].desc->bmAttributes;
            UINT16  max_pkt_size =
                    intf_info->current->endp[i].desc->wMaxPacketSize0;

            endpoint = intf_info->current->endp[i].desc->bEndpointAddress;

            max_pkt_size |=
            ((intf_info->current->endp[i].desc->wMaxPacketSize1) << 8);

            status = NU_USB_HW_Open_Pipe (device->usb_device->hw, 0,
                              endpoint,type, 0, max_pkt_size, 0, 0);
            if (status != NU_SUCCESS)
            {
                roll_back = NU_TRUE;
                break;
            }
        }
    }
    
    if (roll_back == NU_TRUE)
    {
        /* Disable all the endpoints in the new alternate setting.  */
         
        for (i = 0; i < intf_info->current->desc->bNumEndpoints; i++)
        {
            endpoint = intf_info->current->endp[i].desc->bEndpointAddress;
    
            int_status |= NU_USB_HW_Close_Pipe (device->usb_device->hw, 
                                                0, endpoint);
        }
    }

    if (roll_back == NU_TRUE)
    {
        intf_info->current = current_setting;
    }

    if (roll_back == NU_TRUE)
    {
        /* Enable all the endpoints in the old alternate setting.    */
        for (i = 0; i < current_setting->desc->bNumEndpoints; i++)
        {
            UINT8   type = current_setting->endp[i].desc->bmAttributes;
            UINT16  max_pkt_size =
                    current_setting->endp[i].desc->wMaxPacketSize0;

            endpoint = current_setting->endp[i].desc->bEndpointAddress;

            max_pkt_size |=
            ((current_setting->endp[i].desc->wMaxPacketSize1) << 8);

            int_status |= NU_USB_HW_Open_Pipe (device->usb_device->hw, 0,
                              endpoint, type, 0, max_pkt_size, 0, 0);

        }
    }
    
    NU_UNUSED_PARAM (int_status);
    
    if (status == NU_SUCCESS)
    {
        /* Invoke the driver's set_alt_setting callback.     */
        driver = (NU_USB_DRVR *) (intf_info->driver);

        status = NU_USBF_DRVR_Set_Intf (driver,
                                    (NU_USB_STACK *) stack,
                                    device->usb_device,
                                    intf_info, intf_info->current);
    }
    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       usbf_get_configuration
*
* DESCRIPTION
*
*       GET CONFIGURATION request processing.
*
* INPUTS
*
*       device                              Device on which the setup
*                                           packet is received.
*       setup                               The 8 byte setup packet.
*
* OUTPUTS
*
*       Status
*
**************************************************************************/
STATUS  usbf_get_configuration (NU_USBF_DEVICE   *device,
                                NU_USB_SETUP_PKT *setup)
{
        /* Error checking.   */
    if ((device  == NU_NULL) || (setup == NU_NULL))
    {
        return  NU_USB_INVLD_ARG;
    }

    /* Validate the request. 
     * According USB 2.0 Specs Section 9.4.2
     * If wValue != 0, wIndex != 0, or wLength != 1, 
     * then the device behavior is not specified.
     */
    if((setup->wValue != 0)
        || (setup->wIndex != 0)
        || (setup->wLength != 1))
    {
        return  NU_USB_INVLD_ARG;
    }
    USBF_SET_LENGTH (device->ctrl_irp, 1UL);
    USBF_SET_BUFFER (device->ctrl_irp, device->buffer);

    /* Removal of unused parameter warning. */
    NU_UNUSED_PARAM (setup);

    if (device->active_config != NU_NULL)
    {
        device->buffer[0] =
                device->active_config->desc->bConfigurationValue;
    }
    else
    {
        device->buffer[0] = 0;
    }

    return (NU_SUCCESS);
}

/**************************************************************************
*
* FUNCTION
*
*       usbf_set_configuration
*
* DESCRIPTION
*
*       SET CONFIGURATION request processing.
*
* INPUTS
*
*       device                              Device on which the setup
*                                           packet is received.
*       setup                               The 8 byte setup packet.
*
* OUTPUTS
*
*       NU_SUCCESS                          On successful change of
*                                           configuration.
*       NU_USB_INVLD_ARG                    On any other error.
*
**************************************************************************/
STATUS  usbf_set_configuration (NU_USBF_DEVICE   *device,
                                NU_USB_SETUP_PKT *setup)
{
    STATUS              status = NU_SUCCESS;
    INT                 i;
    INT                 j = 0;
    INT                 k = 0;
    UINT8               config_value;
    NU_USB_CFG_DESC    *cfg;
    NU_USB_ALT_SETTG   *alt_settg;
    UINT8               endpoint;
    NU_USB_DEVICE      *usb_device;

    /* Error checking.   */
    if ((device  == NU_NULL) || (setup == NU_NULL) ||
        (device->usb_device == NU_NULL))
    {
        return NU_USB_INVLD_ARG;
    }
    
    /* Validate the request. 
     * According USB 2.0 Specs Section 9.4.7
     * The lower byte of the wValue field specifies the desired 
     * configuration. The upper byte of the wValue field is reserved.
     * If wIndex != 0, wLength != 0, or the upper byte of wValue is 
     * non-zero, then the behavior of this request is not specified.
     */
    if(((setup->wValue) & 0xF0 )!= 0
        || (setup->wIndex != 0)
        || (setup->wLength != 0))
    {
        return  NU_USB_INVLD_ARG;
    }
    usb_device   = device->usb_device;
    config_value = (UINT8) setup->wValue;
    USBF_SET_LENGTH (device->ctrl_irp, 0UL);/* No data phase.            */

    if (config_value == 0)
    {
        if(device->active_config)
        {
            status =  usbf_close_pipes(device,
                                   device->active_config); 
            if (status == NU_SUCCESS)
            {               
               status = usbf_disable_device (device);
            }

            if (status == NU_SUCCESS)
            {
                device->state = USB_STATE_ADDRESSED;
            }
        }
        else
        {
            status = NU_SUCCESS;    
        }
        return (status);
    }

    /* For the compiler to stop warning about 'cfg' being used without
     * initialized.
     */
    cfg = usb_device->config_descriptors[0]->desc;

    /* Search for this configuration in the descriptors. */
    for (i = 0; i < usb_device->device_descriptor.bNumConfigurations; i++)
    {
        cfg = usb_device->config_descriptors[i]->desc;

        if (cfg->bConfigurationValue == config_value)
        {
            break;
        }
    }

    if (i == usb_device->device_descriptor.bNumConfigurations)
    {
        return (NU_USB_INVLD_ARG);
    }
    if (device->active_config != NU_NULL)
    {
        /* Cfg has the device required configuration.
         * If device tries to set the same configuration as is existing.
         * Then do nothing and return
         */
        if (cfg == device->active_config->desc)
        {
            return (NU_SUCCESS);
        }

        /* Disable all endpoints associated with this configuration. */
        status =  usbf_close_pipes(device,
                                   device->active_config);
        if (status == NU_SUCCESS)
        {
            /* Execute all disable callbacks.       */
            status = usbf_disable_device (device);
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Change the configuration.     */
        device->active_config = usb_device->config_descriptors[i];

        /* Reset all interfaces to their default alternate setting.  */
        for (j = 0; j < device->active_config->desc->bNumInterfaces; j++)
        {
            device->active_config->intf[j].driver = NU_NULL;

            alt_settg = device->active_config->intf[j].current =
                &(device->active_config->intf[j].alt_settg[0]);

            /* Enable all endpoints associated with this configuration.  */
            for (k = 0; k < alt_settg->desc->bNumEndpoints; k++)
            {
                UINT8       type = alt_settg->endp[k].desc->bmAttributes;
                UINT16      max_pkt_size =
                                alt_settg->endp[k].desc->wMaxPacketSize0;

                endpoint = alt_settg->endp[k].desc->bEndpointAddress;

                max_pkt_size |=
                        ((alt_settg->endp[k].desc->wMaxPacketSize1) << 8);

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
                if ( usb_device->speed == USB_SPEED_SUPER )
                {
                    UINT8 max_burst;
                    UINT8 ss_ep_attrib;

                    max_burst = alt_settg->endp[j].epcompanion_desc->bMaxBurst;
                    ss_ep_attrib = alt_settg->endp[j].epcompanion_desc->bmAttributes;

                    status |= NU_USB_HW_Open_SS_Pipe (usb_device->hw, 0, endpoint,
                                                type, 0, max_pkt_size, 0, 0, max_burst, ss_ep_attrib, 0);
                }
                else
#endif
                {
                    status |= NU_USB_HW_Open_Pipe (usb_device->hw, 0, endpoint,
                                         type, 0, max_pkt_size, 0, 0);
                }                                     
                if (status != NU_SUCCESS)
                {
                    break;
                }
            }
        }
    }

    /* If pipes are not opened successfully for new configuration then
       set the active configuration to NU_NULL.
    */
    if (status != NU_SUCCESS)
    {
       device->active_config = NU_NULL; 
    }

    /* Update the device state.     */
    if (status == NU_SUCCESS)
    {
        device->state = USB_STATE_CONFIGURED;
        usb_device->active_cnfg_num = (UINT8)i;
    }
    /* Execute required enable callbacks. */
    if (status == NU_SUCCESS) 
    {
        status = usbf_enable_device (device); 
    }

    /* Thats about it!  */
    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       usbf_sync_frame
*
* DESCRIPTION
*
*       SYNC FRAME request processing  function. This is an optional
*       request and is not supported currently.
*
* INPUTS
*
*       device                              Device on which the setup
*                                           packet is received.
*       setup                               The 8 byte setup packet.
*
*
* OUTPUTS
*
*       NU_USB_NOT_SUPPORTED                Always.
*
**************************************************************************/
STATUS  usbf_sync_frame (NU_USBF_DEVICE   *device,
                         NU_USB_SETUP_PKT *setup)
{
    /* Removal of unused parameter warning. */
    NU_UNUSED_PARAM (device);

    /* Removal of unused parameter warning. */
    NU_UNUSED_PARAM (setup);

    /* Sync Frame is not supported. */
    return (NU_USB_NOT_SUPPORTED);
}

/**************************************************************************
*
* FUNCTION
*
*       usbf_setup_descriptor
*
* DESCRIPTION
*
*       Finds descriptors for the operating speed determined during reset 
*       and calls descriptor parsing routine to initialize related data 
*       structures
*
* INPUTS
*       device                  function device to set up its descriptor
*       speed                   operating speed 
*
* OUTPUTS
*       NU_USB_INVLD_DESC       invalid descriptor 
*       NU_SUCCESS              on success
*
**************************************************************************/
STATUS usbf_setup_descriptor(NU_USBF_DEVICE *device, 
                             UINT32 speed)
{
    STATUS          status = NU_SUCCESS;
    INT             i;
    UINT16          size;
    UINT32          capability = 0UL;
    UINT32          num_config;
    NU_USB_DEVICE   *usb_device = NU_NULL; 
    NU_USBF_HW      *fc = NU_NULL;
    UINT8           *config = NU_NULL;
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
    BOOLEAN         spd_out;
#endif

    if ((device  == NU_NULL) || (device->usb_device == NU_NULL) ||
        (device->usb_device->hw == NU_NULL))
    {
        return NU_USB_INVLD_ARG;
    }

    usb_device = device->usb_device;
    fc = (NU_USBF_HW *)(usb_device->hw);

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
    num_config      = 0;
    status          = NU_USB_INVLD_ARG;

    if ( usb_device->raw_bos_descriptors != NU_NULL )
    {
        switch(speed)
        {
            case USB_SPEED_SUPER:
                status = NU_USB_DEVCAP_SuprSpd_Get_SS(usb_device->bos, &spd_out);
                if ( status == NU_SUCCESS && (spd_out) )
                {
                    num_config = usb_device->device_descriptor.bNumConfigurations;
                    device->other_speed = 0;
                    status = NU_SUCCESS;
                }
                break;
            case USB_SPEED_HIGH:
                status = NU_USB_DEVCAP_SuprSpd_Get_HS(usb_device->bos, &spd_out);
                if ( status == NU_SUCCESS && (spd_out) )
                {
                    num_config = usb_device->device_descriptor.bNumConfigurations;
                    device->other_speed = 0;
                    status = NU_SUCCESS;
                }
                break;
            case USB_SPEED_FULL:
                status = NU_USB_DEVCAP_SuprSpd_Get_FS(usb_device->bos, &spd_out);
                if ( status == NU_SUCCESS && (spd_out) )
                {
                    num_config = usb_device->device_descriptor.bNumConfigurations;
                    device->other_speed = 0;
                    status = NU_SUCCESS;
                }
                break;
            default:            
                num_config      = 0;
                break;
        }
    }
#endif

    if((status == NU_SUCCESS) || (speed != USB_SPEED_SUPER))
    {
        /* find descriptors for operating speed */
        if (usb_device->device_qualifier.bLength == 0) /* full-speed device */
        {
            num_config = usb_device->device_descriptor.bNumConfigurations;
            device->other_speed = 0;
        }
        else  /* high-speed capable device */
        {
            if(speed == USB_SPEED_HIGH)
            {
                num_config = usb_device->device_descriptor.bNumConfigurations;
                device->other_speed = 0;
            }
            else
            {
                num_config = usb_device->device_qualifier.bNumConfigurations;
                device->other_speed = 1;
            }
        }

        /* Make sure that status is success before getting out of this block. */
        status = NU_SUCCESS;
    }

    if ( status == NU_SUCCESS )
    {
        /* Initialize status variable with invalid descriptor value. */
        status = NU_USB_INVLD_DESC;
        
        /* Parse through all the configurations and fill the local structures
         * for traversal.
         */
        for (i = 0; i < num_config; i++)
        {
            /* get Configuration descriptor for default speed */
            config = usb_device->raw_descriptors[i][speed];
    
            /* get wTotalSize of Configuration descriptor */
            size = *(config + 2);
            size |= (*(config + 3) << 8);
            status = USB_Parse_Descriptors (usb_device,  config,
                                    usb_device->config_descriptors[i], size);
    
            if (status != NU_SUCCESS)
            {
                break;
            }
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Check the FC capabilities. If it supports 'automatic' response
         * to SET_CONFIGURATION then, relevant state changes and issuing
         * callbacks to the clients are to be done here itself.  otherwise
         * the device goes to the default state.
         */
        status = NU_USBF_HW_Get_Capability (fc, &capability);

        if (status == NU_SUCCESS)
        {
            /* Handles SET_CFG.  */
            if (capability & (0x01 << USB_SET_CONFIGURATION))
            {
                /* If the device supports automatic response to
                 * SET_CONFIGURATION, then,
                 * we move the device to configured state
                 * make the first available configuration active
                 * make alternate setting 0 as active in all interfaces
                 * look out for any drivers for this device
                 */
                device->state = USB_STATE_CONFIGURED;
                device->active_config = usb_device->config_descriptors[0];
                usb_device->active_cnfg_num = 0U;

                for (i = 0; i < device->active_config->desc->bNumInterfaces; i++)
                {
                    device->active_config->intf[i].current =
                        &device->active_config->intf[i].alt_settg[0];
                }
                /* Lookout for any drivers for this device. */
                status = usbf_enable_device (device);
            }
            else if (capability & (0x01 << USB_SET_ADDRESS))
            {
               /* If the device supports automatic response to SET_ADDRESS,
                * then, we move the device to addressed state.
                */
                device->state = USB_STATE_ADDRESSED;
                device->active_config = NU_NULL;
            }
            else
            {
                /* Device in default state. */
                device->state = USB_STATE_DEFAULT;
                device->active_config = NU_NULL;
            }
        }   
    }

    return (status);
}
/**************************************************************************
*
* FUNCTION
*
*       usbf_close_pipes
*
* DESCRIPTION
*        This function closes all the endpoints in a given configuration.
*
* INPUTS
*       device                  Pointer to function device.
*       cfg                     Pointer to the configuration in
                                which endpoints are to be closed.
*
* OUTPUTS
*       NU_NOT_PRESENT          Endpoint is not present
*       NU_SUCCESS              On success
*
**************************************************************************/
STATUS usbf_close_pipes(NU_USBF_DEVICE   *device,
                        NU_USB_CFG   *cfg)
{
    NU_USB_INTF      *intf_info       = NU_NULL;
    NU_USB_ALT_SETTG *current_setting = NU_NULL;
    NU_USB_DEVICE    *usb_device      = NU_NULL;
    STATUS            status          = NU_SUCCESS;
    INT                 i = 0;
    INT                 j = 0;
    UINT8               endpoint;

    if ((device == NU_NULL) || (cfg == NU_NULL))
    {
        return (NU_USB_INVLD_ARG);
    }

    usb_device = device->usb_device;

    /* Go through all the interfaces in a given configuration and close
       the associated endpoints.
     */
    for (i = 0; i < cfg->desc->bNumInterfaces; i++)
    {
        intf_info = &cfg->intf[i];
        current_setting = intf_info->current;

        /* Disable all the endpoints in the current alternate setting.*/
        for (j = 0; j < current_setting->desc->bNumEndpoints; j++)
        {
            endpoint = current_setting->endp[j].desc->bEndpointAddress;

            status = NU_USB_HW_Close_Pipe (usb_device->hw, 
                                           usb_device->function_address,
                                           endpoint);
           if (status != NU_SUCCESS)
           {
               break;
           }
        }
    }

    return status;
}

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
/**************************************************************************
*
* FUNCTION
*
*       usbf_set_isochornous_delay
*
* DESCRIPTION
*
*       SET Isochoronus Delay request processing.
*
* INPUTS
*
*       device                              Device on which the setup
*                                           packet is received.
*       setup                               The 8 byte setup packet.
*
* OUTPUTS
*
*       NU_SUCCESS                          If the interface could be found
*       NU_USB_INVLD_ARG                    For invalid interfaces.
*
**************************************************************************/
STATUS usbf_set_isochornous_delay(NU_USBF_DEVICE   *device,
                                  NU_USB_SETUP_PKT *setup)
{
    /* Removal of unused parameter warning. */
    NU_UNUSED_PARAM (device);

    /* Removal of unused parameter warning. */
    NU_UNUSED_PARAM (setup);

    /* Sync Frame is not supported. */
    return (NU_USB_NOT_SUPPORTED);
}

/**************************************************************************
*
* FUNCTION
*
*       usbf_set_system_exit_latency
*
* DESCRIPTION
*
*       SET System Exit Latency  request processing.
*
* INPUTS
*
*       device                              Device on which the setup
*                                           packet is received.
*       setup                               The 8 byte setup packet.
*
* OUTPUTS
*
*       NU_SUCCESS                          If the interface could be found
*       NU_USB_INVLD_ARG                    For invalid interfaces.
*
**************************************************************************/
STATUS usbf_set_system_exit_latency(NU_USBF_DEVICE   *device,
                                    NU_USB_SETUP_PKT *setup)
{
    
    NU_USB_DEVICE   *usb_device;

    usb_device = (NU_USB_DEVICE*)device;

    device->buffer[0] = usb_device->pw_attrib.sel.u1_sel;
    device->buffer[1] = usb_device->pw_attrib.sel.u1_pel;
    device->buffer[2] = (UINT8) usb_device->pw_attrib.sel.u2_sel;
    device->buffer[3] = (UINT8) (usb_device->pw_attrib.sel.u2_sel >> 8);
    device->buffer[4] = (UINT8) usb_device->pw_attrib.sel.u2_pel;
    device->buffer[5] = (UINT8) (usb_device->pw_attrib.sel.u2_pel >> 8);

    USBF_SET_LENGTH (device->ctrl_irp, setup->wLength);    
    USBF_SET_BUFFER (device->ctrl_irp, device->buffer);
    
    return ( NU_SUCCESS );
}

#endif /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */

/*************************************************************************/ 
#endif  /* USBF_STACK_IMP_C */
/* ======================  End Of File  ================================ */

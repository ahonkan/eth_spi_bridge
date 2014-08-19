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
*       nu_usbf_stack_ext.c
*
* COMPONENT
*
*       Stack component / Nucleus USB Device Software.
*
* DESCRIPTION
*
*       This file contains the implementation of the exported functions for
*       the USB Function Stack component.
*
* DATA STRUCTURES
*
*       usbf_hisr_stack                     HISR Stack.
*
* FUNCTIONS
*
*       NU_USBF_STACK_Attach_Device         Attaches a USB Device to the
*                                           Stack.
*       NU_USBF_STACK_Create                Initializes the stack.
*       _NU_USBF_STACK_Delete               Disables the stack.
*       NU_USBF_STACK_New_Setup             Processes the newly arrived
*                                           SETUP.
*       NU_USBF_STACK_New_Transfer          Processes a newly arrived TOKEN
*       NU_USBF_STACK_Notify                Processes the USB event.
*       NU_USBF_STACK_Speed_Change          Processes a Speed change event.
*       _NU_USBF_STACK_Add_Hw               Registers a HW with the stack.
*       _NU_USBF_STACK_Deregister_Drvr      Deregisters a driver with the
*                                           stack.
*       _NU_USBF_STACK_Flush_Pipe           Flushes all transfers on a pipe
*       _NU_USBF_STACK_Get_Device_Status    Retrieves a device status.
*       _NU_USBF_STACK_Get_Endp_Status      Retrieves an endpoint status.
*       _NU_USBF_STACK_Is_Endp_Stalled      Checks if an endpoint is
*                                           stalled.
*       _NU_USBF_STACK_Register_Drvr        Registers a driver with the
*                                           stack.
*       _NU_USBF_STACK_Remove_Hw            Deregisters a HW with the
*                                           stack.
*       _NU_USBF_STACK_Stall_Endp           Stalls a given endpoint.
*       _NU_USBF_STACK_Submit_IRP           Submits a transfer to a pipe.
*       _NU_USBF_STACK_Unstall_Endp         Un-stalls a given endpoint.
*       _NU_USBF_STACK_End_Session          Wish to end a session.
*       _NU_USBF_STACK_Start_Session        Wish to start a session.
*       _NU_USBF_STACK_Function_Suspend     Nucleus USB Function Stack calls
*                                           this dispatch table function when it
*                                           receives a SET_FEATURE
*                                           (Function_Suspend) request from
*                                           host. Only applicable for stack
*                                           version supporting USB 3.0
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/
#ifndef USBF_STACK_EXT_C
#define USBF_STACK_EXT_C
/* ==============  USB Include Files ==================================  */
#include    "connectivity/nu_usb.h"

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_STACK_Create
*
* DESCRIPTION
*
*       This function initializes the given stack control block.
*       1. Initializes the parent by using its create function
*       2. Initializes its control block variables.
*
* INPUTS
*
*       cb            Stack Control block to be initialized.
*       name          Name of the Stack.
*
* OUTPUTS
*
*       NU_SUCCESS          If Stack could be initialized successfully
*       error code          As returned by called functions otherwise.
*       NU_USB_INVLD_ARG    Indicates the one of the input
*                           parameters has been incorrect
*                           AND/OR the event could not be
*                           processed without an error.
**************************************************************************/
STATUS  NU_USBF_STACK_Create (NU_USBF_STACK *cb,
                              CHAR          *name)
{
    STATUS  status = NU_USB_NOT_SUPPORTED;
    NU_USB_STACK *base_cb = NU_NULL;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Error checking.   */
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(name);
    /* Also check if stack subsystem is already created or not. */
    NU_USB_PTRCHK_RETURN(nu_usbf);

    base_cb = (NU_USB_STACK *)cb;

    /* Create the extended stack using base class method.    */
    memset (cb, 0, sizeof(NU_USBF_STACK));
    status = _NU_USB_STACK_Create ((NU_USB_STACK *) base_cb,
                                   &nu_usbf->stack_subsys,
                                   name, &usbf_stack_dispatch);

    if (status == NU_SUCCESS)
    {
        /* Initialize the control block. */
        cb->num_devices = 0UL;

        ((NU_USB_STACK *) cb)->num_class_drivers = 0UL;

        /* USBF HISR.     */
        cb->usbf_hisr.stack = cb;

        status =
            NU_Create_HISR ((NU_HISR *) (&(cb->usbf_hisr)), "USBF_HISR",
                            usbf_hisr, USBF_HISR_PRIORITY, usbf_hisr_stack,
                            USBF_HISR_STACK_SIZE);

         if (status != NU_SUCCESS)
         {
            _NU_USB_STACK_Delete((VOID*) base_cb);
         }
    }
    /* Execution complete. */

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       _NU_USBF_STACK_Delete
*
* DESCRIPTION
*
*       This function disables the given stack.
*       1. Notifies the registered class drivers of the Stack shutdown
*          event.
*       2. Un-initializes all the associated hardware controllers.
*       3. Calls _NU_USB_Delete() to accomplish any generic removal
*          functionality.
*
* INPUTS
*
*       stack         Stack Control block to be uninitialized.
*
* OUTPUTS
*
*       NU_SUCCESS    If Stack could be uninitialized successfully
*       error code    as returned by called functions otherwise.
*       NU_USB_INVLD_ARG    Indicates the one of the input
*                           parameters has been incorrect
*                           AND/OR the event could not be
*                           processed without an error.
**************************************************************************/
STATUS  _NU_USBF_STACK_Delete (VOID *stack)
{
    STATUS          status = NU_SUCCESS;
    INT             i = 0;
    NU_USB_STACK   *cb;
    NU_USBF_STACK  *usbf_stack;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Error checking.   */
    NU_USB_PTRCHK_RETURN(stack);

    cb = (NU_USB_STACK *) stack;
    usbf_stack = (NU_USBF_STACK *) stack;

    /* Inform all the registered class drivers  of the stack shutdown
     * event.
     */
    for (i = 0; i < cb->num_class_drivers; i++)
    {
        status = NU_USBF_DRVR_Notify (cb->class_drivers[i].driver,
                             cb, NU_NULL, USBF_EVENT_STACK_SHUTDOWN);
    }

    /* Un-initialize the function controllers. */

    for (i = 0; i < usbf_stack->num_devices; i++)
    {
        status = NU_USB_HW_Uninitialize (
                                usbf_stack->device_list[i].usb_device->hw);
    }

    status |= NU_Delete_HISR((NU_HISR *)&usbf_stack->usbf_hisr);

    /* Any generic deletions to be done by the parent. */
    status |= _NU_USB_STACK_Delete (stack);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_STACK_Attach_Device
*
* DESCRIPTION
*
*       This function registers a USB device with the Stack.
*       The Corresponding FC h/w pointer must have already been attached to
*       this device.
*       This function
*       1. adds the device to the list
*       2. initializes the internal data structures for the device
*       3. Parses through the raw-configuration descriptors of the
*       device.
*       4. Checks to see if the device automatically responds to any of
*       the standard USB requests and enables the class drivers
*       accordingly.
*
* INPUTS
*
*       cb            Stack control block.
*       usb_device    Device to be registered.
*
* OUTPUTS
*
*       NU_SUCCESS    If the device is successfully registered with the stack.
*       Error         status as returned by the called functions if any
*       NU_USB_INVLD_ARG    Indicates the one of the input
*                           parameters has been incorrect
*                           AND/OR the event could not be
*                           processed without an error.
**************************************************************************/
STATUS  NU_USBF_STACK_Attach_Device (NU_USBF_STACK *cb,
                                     NU_USB_DEVICE *usb_device)
{
    STATUS          status;
    NU_USBF_DEVICE *device = NU_NULL;
    UINT8           device_index;
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
    UINT8           *raw_bos_desc;
    UINT16          desc_size;
#endif

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Error checking.   */
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(usb_device);

    if (cb->num_devices >= NU_USBF_MAX_HW)
    {
        /* Switch back to user mode. */
        NU_USER_MODE();
        return NU_USB_MAX_EXCEEDED;
    }

    /* locate the index where the device is to be added in the list*/
    for (device_index = 0; device_index < NU_USBF_MAX_HW; device_index++ )
    {
        if (cb->device_list[device_index].usb_device == NU_NULL)
        {
            /* Add this device to the list. */
            device = &cb->device_list[device_index];

            /* Allocate memory for control transfers. */
            status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                        USBF_DEVICE_TXFR_BUFF_SIZE,
                                        (VOID**)&device->buffer);
            if ( status != NU_SUCCESS )
            {
                return ( status );
            }
            
            device->usb_device = usb_device;
            usb_device->stack = (NU_USB_STACK *) cb;
            break;
        }
    }

    if (device_index >= NU_USBF_MAX_HW)
    {
        /* Switch back to user mode. */
        NU_USER_MODE();
        return NU_USB_MAX_EXCEEDED;
    }

    /* Initialize the control IRP. */
    if (device != NU_NULL)
    {
        device->ctrl_irp.version = 0U;
        device->ctrl_irp.callback_data = (VOID *) 0;
        device->ctrl_irp.callback = usbf_ep0_transfer_complete;
    }
    else
    {
        /* Switch back to user mode. */
        NU_USER_MODE();
        return (NU_INVALID_POINTER);
    }

    /* Add this device to the device_list.     */
    cb->num_devices++;
    usb_device->ctrl_pipe.device = usb_device;
    usb_device->ctrl_pipe.endpoint = NU_NULL;

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
    /* If stack is configured for USB 3.0 then parse and save BOS descriptor. */
    if(device->usb_device->raw_bos_descriptors != NU_NULL)
    {
        raw_bos_desc = device->usb_device->raw_bos_descriptors;
                    
        desc_size = *(raw_bos_desc + 2);
        desc_size |= (*(raw_bos_desc + 3) << 8);
                    
        status = USB_Parse_BOS_Descriptor(usb_device, 
                                            raw_bos_desc, 
                                            device->usb_device->bos, 
                                            desc_size);
        if ( status == NU_SUCCESS )
        {
            usb_device->speed = USB_SPEED_UNKNOWN;
        }
    }
#else
    /* determine speed of the device */
    if (usb_device->device_qualifier.bLength == 0)
    {
        usb_device->speed = USB_SPEED_FULL;
        status = usbf_setup_descriptor(device, USB_SPEED_FULL);
    }
    else
    {  /* operating speed of high-speed capable device will be
          determined during reset signaling  */
        usb_device->speed = USB_SPEED_UNKNOWN;
        status = NU_SUCCESS;
    }
#endif

    if (status == NU_SUCCESS)
    {
        /* enable interrupts to detect bus reset  */
        status = NU_USB_HW_Enable_Interrupts ((NU_USB_HW *) usb_device->hw);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}
/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_STACK_Detach_Device
*
* DESCRIPTION
*
*       This function deregisters a USB device with the Stack.
*
* INPUTS
*
*       cb                                  Stack control block.
*       usb_device                          Device to be registered.
*
* OUTPUTS
*
*       NU_SUCCESS                          If the device could be
*                                           successfully deregistered with
*                                           the stack.
*       NU_NOT_PRESENT                      If the device was not earlier
*                                           registered with the stack.
*       NU_USB_INVLD_ARG    Indicates the one of the input
*                           parameters has been incorrect
*                           AND/OR the event could not be
*                           processed without an error.
**************************************************************************/
STATUS  NU_USBF_STACK_Detach_Device (NU_USBF_STACK *cb,
                                     NU_USB_DEVICE *usb_device)
{
    STATUS          status = NU_NOT_PRESENT;
    INT             index;
    NU_USBF_DEVICE  *usbf_dev;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Error checking.   */
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(usb_device);

    /* Search the device in a registered list of function controllers.*/
    for (index= 0x00 ; index < NU_USBF_MAX_HW ; index++)
    {
        if(usb_device == (cb->device_list[index].usb_device))
        {
            /* Get the device pointer.*/
            usbf_dev = &(cb->device_list[index]);

            /* Disable the device.*/
            status = usbf_disable_device(usbf_dev);

            if (status == NU_SUCCESS)
            {
                /* Deallocate memory, allocated for USB transfers. */
                if ( usbf_dev->buffer != NU_NULL )
                {
                    USB_Deallocate_Memory(usbf_dev->buffer);
                    usbf_dev->buffer = NU_NULL;
                }

                memset(&cb->device_list[index],0x00,sizeof(NU_USBF_DEVICE));
                cb->num_devices--;
            }

            break;
        }
    }

    NU_USER_MODE();

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_STACK_New_Setup
*
* DESCRIPTION
*
*       This function processes any SETUP tokens that are sent by the Host,
*       that are not automatically processed by the Hardware.
*       This function :
*
*       1. Initializes the data IRP & the buffer for the device.
*       2. If its a class specific request then appropriate class
*       drivers is handed over the request.
*       3. Validates the standard request against the state of the
*       device.
*       4. Invokes the appropriate request handler.
*
*       At the end of this function, data IRP must have been submitted
*       to the hardware driver, if there is any data transfer. Status phase
*       is automatically handled by the hardware / hardware driver.
*
*       If an error is returned by this function, the Hardware driver
*       automatically stalls the default endpoint to let the Host know of
*       the error situation.
*
* INPUTS
*
*       cb                Stack control block.
*       fc                Hardware on which the setup packet is received.
*       setup             Pointer to the SETUP packet.
*
* OUTPUTS
*
*       NU_SUCCESS        On successful processing of the SETUP packet
*       error status      as returned by the called function otherwise.
*       NU_USB_INVLD_ARG    Indicates the one of the input
*                           parameters has been incorrect
*                           AND/OR the event could not be
*                           processed without an error.
**************************************************************************/
STATUS  NU_USBF_STACK_New_Setup (NU_USB_STACK     *cb,
                                 NU_USBF_HW       *fc,
                                 NU_USB_SETUP_PKT *setup)
{
    NU_USBF_DEVICE *device;
    NU_USB_IRP     *irp;
    STATUS          status;
    INT             index;
    UINT8           b_request;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Error checking.   */
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(fc);
    NU_USB_PTRCHK_RETURN(setup);

    /* find function controller */
    device = usbf_get_device (cb, fc, &index);
    if (device == NU_NULL)
    {
        /* Switch back to user mode. */
        NU_USER_MODE();
        return NU_NOT_PRESENT;
    }

    irp = &(device->ctrl_irp);

    /* Reset the IRP.    */
    memset (device->buffer, 0, sizeof (device->buffer));

    irp->buffer = NU_NULL;
    irp->flags = 0x0UL;
    irp->status = NU_SUCCESS;

    /* Check if the setup is a class/vendor specific setup
     * For GET/SET Descriptor requests, descriptor types (Spec Table 9.5)
     * bigger than 0x1F are considered as class/vendor requests.
     */
    if ((setup->bmRequestType & 0x60) || /* D6..5: 1 = class, 2 = vendor */
        ((setup->wValue & 0xE000) &&
        ((setup->bRequest == USB_GET_DESCRIPTOR) ||
        (setup->bRequest == USB_SET_DESCRIPTOR))))
    {
        status = usbf_process_class_specific (device, setup);
    }
    else /* Standard Request */
    {
         b_request = setup->bRequest;

         /* The bRequest type for SET SEL and SET ISOCH Delay are 48 and 49
          * respectively.They are tranlsted to 13 and 14 to make keep
          * request worker array continuous.
          */
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
        if ( b_request >= SET_SEL )
        {
            b_request = setup->bRequest - 35;
        }
#endif
        /* Validate the request. */
        if ( b_request >= usbf_num_supported_ctrl_requests)
        {
            status = NU_USB_INVLD_ARG;
        }
        else
        {
            status = usbf_validate_setup (device, setup);
        }

        /* Process request.  */
        if (status == NU_SUCCESS)
        {
            status =
                usbf_ctrl_request_worker[b_request] (device, setup);
        }
        if (status == NU_SUCCESS)
        {
            if (irp->length != 0)           /* Data phase.               */
            {
                status = NU_USB_Submit_IRP (cb, irp, &(device->usb_device->ctrl_pipe));
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
    /* A failure here, results in a stall on control endpoint.  */
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_STACK_New_Transfer
*
* DESCRIPTION
*
*       This function is called by the FC when a new token is received on
*       an endpoint for which there is no existing transfer.
*
*       This function determines the class driver to which this endpoint
*       belongs to and invokes the data callback of the class driver with
*       appropriate parameters.
*
*       New transfer requests on the default endpoint 0 are not expected.
*       This is because we require the class drivers and the stack itself
*       to submit any transfers at the time of receiving the SETUP itself.
*
* INPUTS
*
*       cb                  Stack control block.
*       fc                  Hardware on which the new token has been received.
*       bEndpointAddress    Endpoint on which the token has been received.
*
* OUTPUTS
*
*       NU_SUCCESS          On successful processing of the token.
*       NU_NOT_PRESENT      If the device could not be found.
*       NU_USB_INVLD_ARG    If the endpoint could not be found.
*
**************************************************************************/
STATUS  NU_USBF_STACK_New_Transfer (NU_USB_STACK *cb,
                                    NU_USBF_HW   *fc,
                                    UINT8         bEndpointAddress)
{
    NU_USBF_DEVICE *device;
    STATUS          status;
    INT             index;
    NU_USB_PIPE    *pipe;
    NU_USB_DRVR    *driver;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Error checking.   */
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(fc);

    /* Find this function controller */
    device = usbf_get_device (cb, fc, &index);
    if (device == NU_NULL)
    {
        /* Switch back to user mode. */
        NU_USER_MODE();
        return NU_NOT_PRESENT;
    }

    /* Check if this endpoint is valid. i.e., in the descriptors. */
    status = usbf_validate_endpoint (device, bEndpointAddress, &driver);

    /* If the endpoint is valid, we need to inform the associated class
     * driver about this token. Since we talk to the class drivers in terms
     * of pipes, we need to find which pipe does this endpoint belong to.
     * This is done in the only way possible, we go through all the active
     * alternate settings of the interfaces and try to match the endpoint
     * number, to those contained by them.
     * Once the pipe is found, we invoke the new transfer callback onto the
     * driver owning this endpoint.
     */
    if ((status == NU_SUCCESS) && (driver != NU_NULL))
    {
        NU_USB_CFG         *cfg = device->active_config;
        NU_USB_ALT_SETTG   *alt_settg;

        INT                 i = 0;
        INT                 j = 0;

        /* For each of the devices in the active configuration, ... */
        for (i = 0; i < cfg->desc->bNumInterfaces; i++)
        {
            /* Get the active alternate setting.... */
            alt_settg = cfg->intf[i].current;

            /* For each of the endpoints in the alternate setting,.... */
            for (j = 0; j < alt_settg->desc->bNumEndpoints; j++)
            {
                /* Does this endpoint match ?? */
                if (bEndpointAddress ==
                    alt_settg->endp[j].desc->bEndpointAddress)
                {
                    /* Match found. Get the pipe. */
                    pipe = &alt_settg->endp[j].pipe;

                    /* Invoke the new transfer callback of the driver. */
                    status = NU_USBF_DRVR_New_Transfer (driver, cb,
                                               device->usb_device, pipe);


                    /* Switch back to user mode. */
                    NU_USER_MODE();
                    return (status);
                }
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_USB_INVLD_ARG);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_STACK_Notify
*
* DESCRIPTION
*
*       This function is invoked by the Hardware driver whenever there is
*       any USB event, such as RESET or SUSPEND etc.
*       All the registered class drivers are notified of this event
*       through the Notify callback.
*
*       The actual notification capability depends on the hardware. Some
*       hardware may not report the occurrence of these events to the
*       software.
*
* INPUTS
*
*       cb            Stack control block.
*       fc            Hardware on which the event has occurred.
*       event         The event that has occurred.
*
* OUTPUTS
*
*       NU_SUCCESS        On successful notification to class drivers.
*       NU_NOT_PRESENT    If the device could not be found.
*       NU_USB_INVLD_ARG    Indicates the one of the input
*                           parameters has been incorrect
*                           AND/OR the event could not be
*                           processed without an error.
**************************************************************************/
STATUS  NU_USBF_STACK_Notify (NU_USB_STACK *cb,
                              NU_USBF_HW   *fc,
                              UINT32        event)
{
    NU_USBF_DEVICE *device;
    STATUS          status = NU_SUCCESS;
    INT             index;
    UINT32          capability = 0UL;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Error checking.   */
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(fc);

    /* Find the function controller */
    device = usbf_get_device (cb, fc, &index);
    if (device == NU_NULL)
    {
        /* Switch back to user mode. */
        NU_USER_MODE();
        return NU_NOT_PRESENT;
    }

    if (event == USBF_EVENT_RESET)
    {
        /* On a Bus Reset, check the FC capabilities.
         * If it supports 'automatic' response to SET_CONFIGURATION or
         * SET_ADDRESS then, relevant state changes are to be done here
         * itself. Otherwise the device goes to the default state.
         */
        status = NU_USBF_HW_Get_Capability (fc, &capability);

        if (status == NU_SUCCESS)
        {
            /* Handles SET_ADDRESS.  */
            if (capability & (0x01 << USB_SET_ADDRESS))
            {
                /* If the device supports automatic response to SET_ADDRESS,
                * then, we move the device to addressed state.
                */
                device->state = USB_STATE_ADDRESSED;

                /* Handles SET_CFG.  */
                if (capability & (0x01 << USB_SET_CONFIGURATION))
                {
                    /* If the device supports automatic response to
                     * SET_CONFIGURATION, then, we move the device to
                     * configured state.
                     */
                    device->state = USB_STATE_CONFIGURED;
                    device->active_config =
                            device->usb_device->config_descriptors[0];
                }
                else
                {
                    device->active_config = NU_NULL;
                }
            }
        }
    }

    /* Notify the clients of this event. */
    for (index = 0; index < cb->num_class_drivers; index++)
    {
        status = NU_USBF_DRVR_Notify (cb->class_drivers[index].driver,
            cb, device->usb_device, event);
    }

    /* On a connect/disconnect, reset the device state. */
    if ((event == USBF_EVENT_CONNECT) ||
        (event == USBF_EVENT_RESET)||
        (event == USBF_EVENT_DISCONNECT))
    {
        device->state = USB_STATE_DEFAULT;
        device->active_config = NU_NULL;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_STACK_Speed_Change
*
* DESCRIPTION
*
*       This function is invoked by the Hardware driver whenever there is
*       a USB speed change detected.
*       This is required for devices that are dual speed, such as HIGH and
*       FULL speeds.
*       Currently this function doesn't do anything.
*
* INPUTS
*
*       cb            Stack control block.
*       fc            Function controller that has detected the speed change.
*       speed         Current speed of the device.
*
* OUTPUTS
*
*       NU_SUCCESS
*       NU_USB_INVLD_ARG    Indicates the one of the input
*                           parameters has been incorrect
*                           AND/OR the event could not be
*                           processed without an error.
**************************************************************************/
STATUS  NU_USBF_STACK_Speed_Change (NU_USB_STACK    *cb,
                                    NU_USBF_HW      *fc,
                                    UINT32          speed
#if CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE
                                    ,
                                    UINT8           maxp0
#endif
                                    )
{
    STATUS          status;
    NU_USBF_DEVICE  *device;
    INT             index;
    UINT16          bcdUSB;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Error checking.   */
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(fc);
    
    /* Initialize status to invalid value. */
    status = NU_USB_INVLD_ARG;

    device = usbf_get_device (cb, fc, &index);
    if (device)
    {
        switch(speed)
        {
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
            case USB_SPEED_SUPER:
                bcdUSB = USB_VERSION_3_0;
                break;
            case USB_SPEED_HIGH:
                bcdUSB = USB_VERSION_2_1;
                break;
            case USB_SPEED_FULL:
            case USB_SPEED_LOW:
                bcdUSB = USB_VERSION_1_1;
                break;
#endif
            default:
                bcdUSB = USB_VERSION_2_0;
                break;
        }

        /* Update bcdUSB of device. */
        status = NU_USB_DEVICE_Set_bcdUSB(device->usb_device, bcdUSB);
        if ( status == NU_SUCCESS )
        {
#if CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE
            /* Update maximum packet size of default control endpoint. */
            status = NU_USB_DEVICE_Set_bMaxPacketSize0(device->usb_device, maxp0);
            if ( status == NU_SUCCESS )
#endif
            {
                /* Set descriptors according to connected speed. */
                status = usbf_setup_descriptor(device, speed);
                
                /* Save connection speed for later use. */
                device->usb_device->speed =(UINT8) speed;
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( status );
}

/**************************************************************************
*
* FUNCTION
*
*       _NU_USBF_STACK_Add_Hw
*
* DESCRIPTION
*
*       This function attaches a new Hardware (Driver) to the Stack.
*
* INPUTS
*
*       cb            Stack control block.
*       fc            Function controller control block.
*
* OUTPUTS
*
*       NU_SUCCESS    If the FC could be registered with the stack
*       Error         Status returned by the called functions otherwise.
*       NU_USB_INVLD_ARG    Indicates the one of the input
*                           parameters has been incorrect
*                           AND/OR the event could not be
*                           processed without an error.
**************************************************************************/
STATUS  _NU_USBF_STACK_Add_Hw (NU_USB_STACK *cb,
                               NU_USB_HW    *fc)
{
    STATUS  status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Error checking.   */
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(fc);

    /* Initialize the device.    */
    status = NU_USB_HW_Initialize ((NU_USB_HW *) fc, cb);

    if (status == NU_SUCCESS)
    {
        /* Disable interrupts from this FC.  */
        status = NU_USB_HW_Disable_Interrupts ((NU_USB_HW *) fc);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       _NU_USBF_STACK_Remove_Hw
*
* DESCRIPTION
*
*       Removes a registered FC from the Stack.
*
* INPUTS
*
*       cb              Stack control block.
*       fc              FC (control block) to be removed.
*
* OUTPUTS
*
*       NU_SUCCESS      If the controller could be deregistered with the stack
*       NU_NOT_PRESENT  If the device could not be found.
*       NU_USB_INVLD_ARG    Indicates the one of the input
*                           parameters has been incorrect
*                           AND/OR the event could not be
*                           processed without an error.
**************************************************************************/
STATUS  _NU_USBF_STACK_Remove_Hw (NU_USB_STACK *cb,
                                  NU_USB_HW    *fc)
{
    STATUS          status;
    NU_USBF_DEVICE *device;
    INT             devindex = 0;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Error checking.   */
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(fc);

    /* Disable any further interrupts from this controller  */
    status = NU_USB_HW_Disable_Interrupts ((NU_USB_HW *) fc);

    /* Disable callback for the controller. */
    status |= NU_USB_HW_Uninitialize ((NU_USB_HW *) fc);

    /* Get the device. */
    device = usbf_get_device (cb, (NU_USBF_HW *) fc, &devindex);

    if (device != NU_NULL)
    {
        device->state = USB_STATE_POWERED;
        device->active_config = NU_NULL;
    }

    else
    {
        status |= NU_NOT_PRESENT;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}
/*************************************************************************
* FUNCTION
*               _NU_USBF_STACK_Register_Drvr
*
* DESCRIPTION
*       This function registers new class drivers to the function stack.
*
* INPUTS
*       cb              pointer to function stack control block
*       class_driver    pointer to class driver control block to be
*                       registered.
*
* OUTPUTS
*   NU_SUCCESS              Indicates that the driver has been successfully
*                           deregistered.
*   NU_USB_INVLD_ARG        Indicates that one or more arguments passed to
*                           this function are invalid.
*
*************************************************************************/
STATUS  NU_USBF_STACK_Register_Drvr (NU_USBF_STACK *cb,
                                     NU_USB_DRVR  *driver)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Parameters validation.*/
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(driver);

    /* Call the base stack register function.*/
    status = NU_USB_STACK_Register_Drvr ((NU_USB_STACK *) cb,
                                          driver);

    NU_USER_MODE();

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       _NU_USBF_STACK_Deregister_Drvr
*
* DESCRIPTION
*
*       Removes a registered class driver from the Stack.
*
* INPUTS
*
*       cb            Stack control block.
*       driver        Class driver (control block) to be removed.
*
* OUTPUTS
*
*       NU_SUCCESS    If the driver could be deregistered with the stack
*       Error         status returned by the called functions otherwise.
*       NU_USB_INVLD_ARG    Indicates the one of the input
*                           parameters has been incorrect
*                           AND/OR the event could not be
*                           processed without an error.
**************************************************************************/
STATUS  _NU_USBF_STACK_Deregister_Drvr (NU_USB_STACK *cb,
                                        NU_USB_DRVR  *driver)
{
    STATUS  status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Error checking.   */
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(driver);

    /* Remove this driver as an active driver from all interfaces
     * And try to identify new drivers for these interfaces.
     */
    status = usbf_disable_driver ((NU_USBF_STACK *) cb, driver);

    if (status == NU_SUCCESS)
    {
        /* Remove this driver from the list. */
        status = _NU_USB_STACK_Deregister_Drvr (cb, driver);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/* Endpoint STALL/UNSTALL API.   */

/**************************************************************************
*
* FUNCTION
*
*       _NU_USBF_STACK_Stall_Endp
*
* DESCRIPTION
*
*       Endpoint Stall API.
*
* INPUTS
*
*       cb            Stack control block.
*       pipe          Pipe of the endpoint to be stalled.
*
* OUTPUTS
*
*       NU_SUCCESS    If the stall is successful.
*       Error         Status as returned by the called functions otherwise.
*
**************************************************************************/
STATUS  _NU_USBF_STACK_Stall_Endp (NU_USB_STACK *cb,
                                   NU_USB_PIPE  *pipe)
{
    NU_USBF_HW *fc;
    UINT8       bEndpointAddress;
    STATUS      status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Removal of unused parameter warning. */
    NU_UNUSED_PARAM (cb);

    /* Error checking.   */
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(pipe);

    /* Retrieve  the FC Control block from the pipe. */
    fc = (NU_USBF_HW *) (pipe->device->hw);

    /* For default endpoints, the address is to be filled specially
     * The convention is that for default pipe, the endpoint member
     * in the pipe structure is filled in as NU_NULL. Thus, the following
     * check is necessary.
     */
    if (pipe->endpoint == NU_NULL)
    {
        bEndpointAddress = 0U;
    }
    else
    {
        bEndpointAddress = pipe->endpoint->desc->bEndpointAddress;
    }

    /* Stall the endpoint.   */
    status = NU_USBF_HW_Stall_Endpoint (fc, bEndpointAddress);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       _NU_USBF_STACK_Unstall_Endp
*
* DESCRIPTION
*
*       Endpoint Un-stall API.
*
* INPUTS
*
*       cb            Stack control block.
*       pipe          Pipe of the endpoint to be un-stalled.
*
* OUTPUTS
*
*       NU_SUCCESS    If the un-stall is successful.
*       Error         Status as returned by the called functions otherwise.
*       NU_USB_INVLD_ARG    Indicates the one of the input
*                           parameters has been incorrect
*                           AND/OR the event could not be
*                           processed without an error.
**************************************************************************/
STATUS  _NU_USBF_STACK_Unstall_Endp (NU_USB_STACK *cb,
                                     NU_USB_PIPE  *pipe)
{
    NU_USBF_HW *fc;
    STATUS      status;
    UINT8       bEndpointAddress;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Removal of unused parameter warning. */
    NU_UNUSED_PARAM (cb);

    /* Error checking.   */
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(pipe);

    /* Retrieve  the FC Control block from the pipe. */
    fc = (NU_USBF_HW *) (pipe->device->hw);

    /* For default endpoints, the address is to be filled specially
     * The convention is that for default pipe, the endpoint member
     * in the pipe structure is filled in as NU_NULL. Thus, the following
     * check is necessary.
     */
    if (pipe->endpoint == NU_NULL)
    {
        bEndpointAddress = 0U;
    }
    else
    {
        bEndpointAddress = pipe->endpoint->desc->bEndpointAddress;
    }

    /* Un-stall the endpoint.   */
    status = NU_USBF_HW_Unstall_Endpoint (fc, bEndpointAddress);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       _NU_USBF_STACK_Is_Endp_Stalled
*
* DESCRIPTION
*
*       Endpoint stall status retrieval API.
*
* INPUTS
*
*       cb                     Stack control block.
*       pipe                   Pipe of the endpoint to be checked for stall.
*       endpoint_status_out    Stall status is returned here.
*
* OUTPUTS
*
*       NU_SUCCESS       If the service is successful.
*       Error            status as returned by the called functions otherwise.
*       NU_USB_INVLD_ARG    Indicates the one of the input
*                           parameters has been incorrect
*                           AND/OR the event could not be
*                           processed without an error.
**************************************************************************/
STATUS  _NU_USBF_STACK_Is_Endp_Stalled (NU_USB_STACK *cb,
                                        NU_USB_PIPE  *pipe,
                                        DATA_ELEMENT *endpoint_status_out)
{
    NU_USBF_HW *fc;
    STATUS      status;
    UINT8       bEndpointAddress;
    UINT16      ep_status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Removal of unused parameter warning. */
    NU_UNUSED_PARAM (cb);

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(endpoint_status_out);
    NU_USB_PTRCHK_RETURN(pipe);

    /* Retrieve  the FC Control block from the pipe. */
    fc = (NU_USBF_HW *) (pipe->device->hw);

    /* For default endpoints, the address is to be filled specially
     * The convention is that for default pipe, the endpoint member
     * in the pipe structure is filled in as NU_NULL. Thus, the following
     * check is necessary.
     */
    if (pipe->endpoint == NU_NULL)
    {
        bEndpointAddress = 0U;
    }
    else
    {
        bEndpointAddress = pipe->endpoint->desc->bEndpointAddress;
    }

    /* Get the endpoint status. */
    *endpoint_status_out = 0;
    status = NU_USBF_HW_Get_Endpoint_Status (fc, bEndpointAddress,
                                            &ep_status);
    if (ep_status)
    {
        *endpoint_status_out = 1;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       _NU_USBF_STACK_Get_Endp_Status
*
* DESCRIPTION
*
*       Endpoint status retrieval API.
*
* INPUTS
*
*       cb                    Stack control block
*       pipe                  Pipe of the endpoint to be checked for status.
*       endpoint_status_out   Endpoint status is returned here.
*
* OUTPUTS
*
*       NU_SUCCESS       If the service is successful.
*       Error            status as returned by the called functions otherwise.
*       NU_USB_INVLD_ARG    Indicates the one of the input
*                           parameters has been incorrect
*                           AND/OR the event could not be
*                           processed without an error.
**************************************************************************/
STATUS  _NU_USBF_STACK_Get_Endp_Status (NU_USB_STACK *cb,
                                        NU_USB_PIPE  *pipe,
                                        UINT16       *endpoint_status_out)
{
    NU_USBF_HW *fc;
    STATUS      status;
    UINT8       bEndpointAddress;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE(); 

    /* Removal of unused parameter warning. */
    NU_UNUSED_PARAM (cb);

    /* Error checks. */
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(endpoint_status_out);
    NU_USB_PTRCHK_RETURN(pipe);

    /* Retrieve  the FC Control block from the pipe. */
    fc = (NU_USBF_HW *) (pipe->device->hw);

    /* For default endpoints, the address is to be filled specially
     * The convention is that for default pipe, the endpoint member
     * in the pipe structure is filled in as NU_NULL. Thus, the following
     * check is necessary.
     */
    if (pipe->endpoint == NU_NULL)
    {
        bEndpointAddress = 0U;
    }
    else
    {
        bEndpointAddress = pipe->endpoint->desc->bEndpointAddress;
    }

    /* Get the endpoint status. */
    status =
        NU_USBF_HW_Get_Endpoint_Status (fc, bEndpointAddress,
                                        endpoint_status_out);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       _NU_USBF_STACK_Get_Device_Status
*
* DESCRIPTION
*
*       Device status retrieval API.
*
* INPUTS
*
*       cb            Stack control block.
*       device        Device to be checked for status.
*       status_out    Device status is returned here.
*
* OUTPUTS
*
*       NU_SUCCESS    If the service is successful.
*       Error         status as returned by the called functions otherwise.
*       NU_USB_INVLD_ARG    Indicates the one of the input
*                           parameters has been incorrect
*                           AND/OR the event could not be
*                           processed without an error.
**************************************************************************/
STATUS  _NU_USBF_STACK_Get_Device_Status (NU_USB_STACK  *cb,
                                          NU_USB_DEVICE *device,
                                          UINT16        *status_out)
{
    NU_USBF_HW *fc;
    STATUS      status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Removal of unused parameter warning. */
    NU_UNUSED_PARAM (cb);

    /* Error checks. */
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(status_out);
    NU_USB_PTRCHK_RETURN(device);

    /* Retrieve  the FC Control block from the pipe. */
    fc = (NU_USBF_HW *) (device->hw);

    /* Get the status. */
    status = NU_USBF_HW_Get_Status (fc, status_out);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/* Data Transfers.   */

/**************************************************************************
*
* FUNCTION
*
*       _NU_USBF_STACK_Submit_IRP
*
* DESCRIPTION
*
*       Data transfer submission API.
*
* INPUTS
*
*       cb            Stack control block.
*       irp           Data transfer description.
*       pipe          Pipe on which the data transfer is to take place.
*
* OUTPUTS
*
*       NU_SUCCESS    If the service is successful.
*       Error         status as returned by the called functions otherwise.
*       NU_USB_INVLD_ARG    Indicates the one of the input
*                           parameters has been incorrect
*                           AND/OR the event could not be
*                           processed without an error.
**************************************************************************/
STATUS  _NU_USBF_STACK_Submit_IRP (NU_USB_STACK *cb,
                                   NU_USB_IRP   *irp,
                                   NU_USB_PIPE  *pipe)
{
    STATUS      status;
    NU_USBF_HW *fc;
    UINT8       bEndpointAddress;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Removal of unused parameter warning. */
    NU_UNUSED_PARAM (cb);

    /* Error Checking */
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(irp);
    NU_USB_PTRCHK_RETURN(pipe);

    fc = (NU_USBF_HW *) (pipe->device->hw);

    /* For default endpoints, the address is to be filled specially
     * The convention is that for default pipe, the endpoint member
     * in the pipe structure is filled in as NU_NULL. Thus, the following
     * check is necessary.
     */
    if (pipe->endpoint == NU_NULL)
    {
        bEndpointAddress = 0U;
    }
    else
    {
        bEndpointAddress = pipe->endpoint->desc->bEndpointAddress;
    }
    status =
         NU_USB_HW_Submit_IRP ((NU_USB_HW *) fc, irp, 0, bEndpointAddress);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       _NU_USBF_STACK_Cancel_IRP
*
* DESCRIPTION
*
*       Data transfer cancellation API.
*
* INPUTS
*
*       cb            Stack control block.
*       pipe          Pipe on which the data transfer is being cancelled.
*
* OUTPUTS
*
*       NU_SUCCESS    If the service is successful.
*       Error         status as returned by the called functions otherwise.
*       NU_USB_INVLD_ARG    Indicates the one of the input
*                           parameters has been incorrect
*                           AND/OR the event could not be
*                           processed without an error.
**************************************************************************/
STATUS  _NU_USBF_STACK_Cancel_IRP (NU_USB_STACK *cb,
                                   NU_USB_PIPE  *pipe)
{
    STATUS      status;
    NU_USBF_HW *fc;
    UINT8       bEndpointAddress;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Removal of unused parameter warning. */
    NU_UNUSED_PARAM (cb);

        /* Error Checking */
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(pipe);

    fc = (NU_USBF_HW *) (pipe->device->hw);

    /* For default endpoints, the address is to be filled specially
     * The convention is that for default pipe, the endpoint member
     * in the pipe structure is filled in as NU_NULL. Thus, the following
     * check is necessary.
     */
    if (pipe->endpoint == NU_NULL)
    {
        bEndpointAddress = 0U;
    }
    else
    {
        bEndpointAddress = pipe->endpoint->desc->bEndpointAddress;
    }
    status = NU_USB_HW_Flush_Pipe ((NU_USB_HW *) fc, 0, bEndpointAddress);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       _NU_USBF_STACK_Flush_Pipe
*
* DESCRIPTION
*
*       Data transfer cancellation API.
*
* INPUTS
*
*       cb            Stack control block.
*       pipe          Pipe on which the data transfer is to be canceled.
*
* OUTPUTS
*
*       NU_SUCCESS    If the service is successful.
*       Error         status as returned by the called functions otherwise.
*       NU_USB_INVLD_ARG    Indicates the one of the input
*                           parameters has been incorrect
*                           AND/OR the event could not be
*                           processed without an error.
**************************************************************************/
STATUS  _NU_USBF_STACK_Flush_Pipe (NU_USB_STACK *cb,
                                   NU_USB_PIPE  *pipe)
{
    STATUS      status;
    UINT8       bEndpointAddress;
    NU_USBF_HW *fc;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Removal of unused parameter warning. */
    NU_UNUSED_PARAM (cb);

    /* Error checks. */
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(pipe);

    fc = (NU_USBF_HW *) (pipe->device->hw);

    /* For default endpoints, the address is to be filled specially
     * The convention is that for default pipe, the endpoint member
     * in the pipe structure is filled in as NU_NULL. Thus, the following
     * check is necessary.
     */
    if (pipe->endpoint == NU_NULL)
    {
        bEndpointAddress = 0U;
    }
    else
    {
        bEndpointAddress = pipe->endpoint->desc->bEndpointAddress;
    }
    status = NU_USB_HW_Flush_Pipe ((NU_USB_HW *) fc, 0, bEndpointAddress);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       _NU_USBF_STACK_Start_Session
*
* DESCRIPTION
*
*       This function is invoked by Applications when they wish to request
*       a session. It checks if the SRP is enabled and invokes hardware
*       drivers Start session service.
*
* INPUTS
*
*       cb         Stack control block.
*       fc         Hardware on which SRP is to be initiated.
*       port_id    Port identifier on which SRP is to be initiated.
*       delay      Time in ms for which SRP is attempted.
*
* OUTPUTS
*
*       NU_SUCCESS              The service could be executed successfully.
*       NU_USB_INVLD_ARG        Specified port could not be found.
*       NU_USB_NOT_SUPPORTED    Not supported by the controller.
*
**************************************************************************/
STATUS  _NU_USBF_STACK_Start_Session (NU_USB_STACK *cb,
                                      NU_USB_HW    *fc,
                                      UINT8         port_id,
                                      UINT16        delay)
{
    STATUS          status = NU_SUCCESS;
    UINT8           role;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Error Checking */
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(fc);

    /* Check if we are a B-device and if we are in the device role */
    status = NU_USB_HW_Get_Role(fc, port_id, &role);

    if (status == NU_SUCCESS)
    {
        if ((role & 0x02) != 0x00)
        {
            status = NU_USB_INVLD_ARG;
        }
        else
        {
            /* Call HW driver's service. */
            status = NU_USB_HW_Start_Session(fc, port_id, delay);
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       _NU_USBF_STACK_End_Session
*
* DESCRIPTION
*
*       This function is invoked by Applications when they wish to end a
*       session.
*
* INPUTS
*
*       cb         Stack control block.
*       fc         Hardware on which SRP is to be initiated.
*       port_id    port identifier on which SRP is to be initiated.
*
* OUTPUTS
*
*       NU_SUCCESS              The service could be executed successfully.
*       NU_USB_INVLD_ARG        Specified port could not be found.
*       NU_USB_NOT_SUPPORTED    Not supported by the controller
*       NU_NOT_PRESENT          If the device could not be found.
*
**************************************************************************/
STATUS  _NU_USBF_STACK_End_Session (NU_USB_STACK *cb,
                                    NU_USB_HW    *fc,
                                    UINT8         port_id)
{
    NU_USBF_DEVICE *device;
    INT             index;
    STATUS          status;
    UINT8           role;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Error checks. */
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(fc);

    /* Get the NU_USBF_DEVICE pointer. */
    device = usbf_get_device(cb, (NU_USBF_HW *)fc, &index);
    if (device == NU_NULL)
    {
        /* Switch back to user mode. */
        NU_USER_MODE();
        return NU_NOT_PRESENT;
    }

    /* Check if SRP is supported by the device. */
    if (!(device->usb_device->otg_status & 0x01))
    {
        status = NU_USB_NOT_SUPPORTED;
    }
    else
    {
        /* Check if we are a B-device and if we are in the  device  role.
         */
        status = NU_USB_HW_Get_Role(fc, port_id, &role);
        if (status == NU_SUCCESS)
        {
            if ((role & 0x07) != 0x01)
            {
                status = NU_USB_INVLD_ARG;
            }
            else
            {
                /* Call HW driver's service. */
                status = NU_USB_HW_End_Session(fc, port_id);
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/* Following functions should only be visible when stack is configured
 * for USB 3.0. */
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

/**************************************************************************
*
*   FUNCTION
*
*       _NU_USBF_STACK_Function_Suspend
*
*   DESCRIPTION
*
*       Nucleus USB Function Stack calls this dispatch table function when
*       it receives a SET_FEATURE(Function_Suspend) request from host.
*
*   INPUTS
*
*       cb                      Pointer to NU_USB_STACK control block.
*       intf                    Pointer to NU_USB_INTF control block.
*       func_suspend            Boolean, containing value of function
*                               suspend option.
*       rmt_wakeup              Boolean, containing value of remote wakeup.
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation Completed Successfully. Interface
*                               is notified that host wants to suspend it.
*       NU_NOT_SUPPORTED        Interface is not present.
*       NU_USB_INVLD_ARG        Any of the input argument is invalid.
*
**************************************************************************/
STATUS _NU_USBF_STACK_Function_Suspend (NU_USB_STACK *cb,
                                        NU_USB_INTF *intf,
                                        BOOLEAN func_suspend,
                                        BOOLEAN rmt_wakeup)
{
    STATUS status = NU_USB_INVLD_ARG;
    UINT32 event;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    if ((cb != NU_NULL) && (intf != NU_NULL))
    {
        event = USBF_EVENT_FUNCTION_SUSPEND;

        /* If function suspend is true, then set the MS-bit (31)*/
        if (rmt_wakeup == NU_TRUE)
            event |= USBF_EVENT_FUNCTION_SUSPEND_RW;

        /* If function suspend is true, then set the second MS-bit (30) */
        if (func_suspend == NU_TRUE)
            event |= USBF_EVENT_FUNCTION_SUSPEND_FS;

        status = NU_USBF_DRVR_Notify(intf->device->driver, cb,
                                                     intf->device, event);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
*
*   FUNCTION
*
*       NU_USBF_STACK_Start_Link_Transition
*
*   DESCRIPTION
*
*       This function initiates link transition when the idle conditions
*       for device endpoints are met. This function is called  by the
*       controller driver only for devcie initiated link transitions.
*
*   INPUTS
*
*       cb                      Pointer to NU_USBF_STACK control block.
*       device                  Pointer to NU_USB_DEVICE control block.
*       idle_period             Idle period provided by the controller
*                               driver.
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation Completed Successfully. 
*       NU_USB_INVLD_ARG        Any of the input argument is invalid.
*       NU_USB_NOT_SUPPORTED    Device is not enabled to initiate link
*                               transition.
**************************************************************************/
STATUS NU_USBF_STACK_Start_Link_Transition (NU_USBF_STACK *cb,
                                            NU_USB_DEVICE *device,
                                            UINT16        idle_period)
{
    STATUS status;
    NU_USB_DEV_PW_ATTRIB *pwr_attrib;
    UINT16 link_state;
    UINT8  power_mode;
    
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();
    
    NU_UNUSED_PARAM(cb);
    NU_USB_PTRCHK_RETURN(device);

    status     = NU_SUCCESS;
    pwr_attrib = NU_NULL;
    link_state = 0;
    power_mode = 0;

    status = NU_USB_PMG_Get_Dev_Pwr_Attrib(device, &pwr_attrib);

    if (status == NU_SUCCESS)
    {
        if (pwr_attrib->u1_enable || pwr_attrib->u1_enable)
        { 
        /* Here we check the value of idle period,there are two conditions
         *
         * 1.If idle period is greater then u2_pel then device is going to
         *   transition its link to U2 state hence we set values link_state
         *   and power mode accordingly.
         * 2.If idle period is less then u2_pel then device is going to
         *   transition its link to U1 state hence we set values link_state
         *   and power mode accordingly.
         *
         * For details refer to USB 3.0 specs,appendix C ,section C 3.2.
         */
            if ((idle_period > pwr_attrib->sel.u2_pel)
                && ( pwr_attrib->u2_enable))
            {
                link_state = USB_LINK_STATE_U2;
                power_mode = USB_POWER_SAVING_MODE2;
    
            }
            else if (pwr_attrib->u1_enable)
            {
                link_state = USB_LINK_STATE_U1;
                power_mode = USB_POWER_SAVING_MODE1;
            }
    
            if (pwr_attrib->ltm_enable)
            {
                /* Generate LTM with new BELT value. */
                status = NU_USB_PMG_Generate_LTM(device, link_state);
            }
    
            if (status == NU_SUCCESS)
            {
                /* Update the power mode. */
                status = NU_USB_HW_Update_Power_Mode(device->hw,
                                                     power_mode);
            }
        }

        else
        {
            status = NU_USB_NOT_SUPPORTED;
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

#endif /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */
#endif /* USBF_STACK_EXT_C */
/* ======================  End Of File  ================================ */

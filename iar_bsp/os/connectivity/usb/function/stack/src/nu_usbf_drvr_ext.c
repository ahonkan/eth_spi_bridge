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
*       nu_usbf_drvr_ext.c
*
*
* COMPONENT
*
*       Stack component / Nucleus USB Device Software.
*
* DESCRIPTION
*
*       This file contains the exported function implementations
*       for the Nucleus USB Software class driver.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_USBF_DRVR_New_Setup              Class specific request handling
*       NU_USBF_DRVR_New_Transfer           New Transfer handling.
*       NU_USBF_DRVR_Notify                 USB Event processing.
*       NU_USBF_DRVR_Set_Intf               New alternate setting
*                                           processing.
*       _NU_USBF_DRVR_Create                Base class driver
*                                           initialization.
*       _NU_USBF_DRVR_Delete                Un-initializes the Base class
*                                           driver.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions
*
**************************************************************************/
#ifndef _USBF_DRVR_EXT_C
#define _USBF_DRVR_EXT_C
/* ==============  USB Include Files ==================================  */

#include    "connectivity/nu_usb.h"

/**************************************************************************
*
* FUNCTION
*
*       _NU_USBF_DRVR_Create
*
* DESCRIPTION
*
*       Initializes the USB Function Stack class driver
*       The parameters describe the credential of the devices that the
*       class driver is intending to service. For a given class driver, all
*       fields may not be valid. The valid values are indicated by the
*       match_flag parameter.
*
* INPUTS
*
*       cb                                  Class driver control block.
*       name                                Name.
*       match_flag                          Defines the validity of the
*                                           following parameters
*       idVendor                            The 16bit USBF assigned vendor
*                                           identification number.
*       idProduct                           The 16 bit product
*                                           identification number.
*       bcdDeviceLow                        The valid lower BCD value for
*                                           the device release number.
*       bcdDeviceHigh                       The valid upper BCD value for
*                                           the device release number.
*       bInterfaceClass                     USB defined Interface class
*                                           code.
*       bInterfaceSubClass                  USB defined Interface subclass
*                                           code.
*       bInterfaceProtocol                  USB defined Interface protocol
*                                           code.
*       dispatch                            Dispatch table of the class
*                                           driver.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the driver has
*                                           been initialized successfully.
*       NU_USB_INVLD_ARG                    Indicates that one of the
*                                           parameters has been incorrect.
*       NU_NOT_PRESENT                      Indicates that there is an
*                                           internal error.
*
**************************************************************************/
STATUS  _NU_USBF_DRVR_Create (NU_USBF_DRVR *cb,
                              CHAR         *name,
                              UINT32        match_flag,
                              UINT16        idVendor,
                              UINT16        idProduct,
                              UINT16        bcdDeviceLow,
                              UINT16        bcdDeviceHigh,
                              UINT8         bInterfaceClass,
                              UINT8         bInterfaceSubClass,
                              UINT8         bInterfaceProtocol,
                              const VOID   *dispatch)
{
    STATUS  status;

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(name);
    NU_USB_PTRCHK(dispatch);

    status = _NU_USB_DRVR_Create ((NU_USB_DRVR *) cb,
                                  &nu_usbf->class_driver_subsys,
                                  name, match_flag,
                                  idVendor, idProduct,
                                  bcdDeviceLow, bcdDeviceHigh,
                                  bInterfaceClass,
                                  bInterfaceSubClass,
                                  bInterfaceProtocol, dispatch);
    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_DRVR_Set_Intf
*
* DESCRIPTION
*
*       Invokes the set_interface worker function for the supplied class
*       driver.
*
* INPUTS
*
*       cb                                  Class driver for which this
*                                           callback is meant for.
*       stack                               Stack invoking this function.
*       device                              Device on which this event has
*                                           happened.
*       intf                                Interface which is affected.
*       alt_settg                           New alternate setting for the
*                                           interface.
*
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the class driver
*                                           could process the event
*                                           successfully.
*       NU_USB_INVLD_ARG                    Indicates the one of the input
*                                           parameters has been incorrect
*                                           AND/OR the event could not be
*                                           processed without an error
*
**************************************************************************/
STATUS  NU_USBF_DRVR_Set_Intf (NU_USB_DRVR      *cb,
                               NU_USB_STACK     *stack,
                               NU_USB_DEVICE    *device,
                               NU_USB_INTF      *intf,
                               NU_USB_ALT_SETTG *alt_settg)
{
    STATUS  status = NU_USB_NOT_SUPPORTED;

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(device);
    NU_USB_PTRCHK(stack);
    NU_USB_PTRCHK(intf);
    NU_USB_PTRCHK(alt_settg);

    /* Invoke the dispatch table worker function. */
    if (((NU_USBF_DRVR_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
        Set_Intf != NU_NULL)
    {
        status =
            ((NU_USBF_DRVR_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
            Set_Intf (cb, stack, device, intf, alt_settg);
    }

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_DRVR_New_Setup
*
* DESCRIPTION
*
*       Processes a new class specific SETUP packet from the Host.
*
*       The format of the setup packet is defined by the corresponding
*       class specification / custom protocol. The setup packet is
*       validated by this function and processed further as per the class
*       specification.
*
*       If there is any data phase associated with the setup request,
*       then the NU_USB_Submit_Irp function can be used to submit the
*       transfer to the stack. Status phase is automatically handled by the
*       Stack. If there is no data transfer associated with the command,
*       then no transfer is submitted.
*
*       For unknown and unsupported command, this function returns
*       appropriate error status. If this function returns any status other
*       than NU_SUCCESS, then the default endpoint will be stalled.
*
* INPUTS
*
*       cb                                  Class driver for which this
*                                           callback is meant for.
*       stack                               Stack invoking this function.
*       device                              Device on which this event has
*                                           happened.
*       setup                               the 8 byte setup packet
*                                           originating from the Host.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the class driver
*                                           could process the event
*                                           successfully.
*       NU_USB_INVLD_ARG                    Indicates the one of the input
*                                           parameters has been incorrect
*                                           AND/OR the event could not be
*                                           processed without an error.
*
**************************************************************************/
STATUS  NU_USBF_DRVR_New_Setup (NU_USB_DRVR      *cb,
                                NU_USB_STACK     *stack,
                                NU_USB_DEVICE    *device,
                                NU_USB_SETUP_PKT *setup)
{
    STATUS  status = NU_USB_NOT_SUPPORTED;

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(device);
    NU_USB_PTRCHK(setup);
    NU_USB_PTRCHK(stack);

    /* Invoke the dispatch table worker function. */
    if (((NU_USBF_DRVR_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
        New_Setup != NU_NULL)
    {
        status =
            ((NU_USBF_DRVR_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
            New_Setup (cb, stack, device, setup);
    }

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_DRVR_New_Transfer
*
* DESCRIPTION
*
*       Processes the new transfer request from the Host.
*
*       If there is any data transfer pending with the class driver,
*       then the NU_USB_Submit_Irp function can be used to submit the
*       transfer to the stack. If there is no data transfer associated
*       with the command, then no transfer is submitted.
*
*       This function is never invoked on default control endpoint. Since
*       stack ignores the return value of this function, any required error
*       processing is carried out by the class driver itself.
*
*
* INPUTS
*
*       cb                                  Class driver for which this
*                                           callback is meant for.
*       stack                               Stack invoking this function.
*       device                              Device on which this event has
*                                           happened.
*       pipe                                Pipe on which the data transfer
*                                           is initiated by the Host.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the class driver
*                                           could process the event
*                                           successfully.
*       NU_USB_INVLD_ARG                    Indicates the one of the input
*                                           parameters has been incorrect
*                                           AND/OR the event could not be
*                                           processed without an error.
*
**************************************************************************/
STATUS  NU_USBF_DRVR_New_Transfer (NU_USB_DRVR   *cb,
                                   NU_USB_STACK  *stack,
                                   NU_USB_DEVICE *device,
                                   NU_USB_PIPE   *pipe)
{
    STATUS  status = NU_USB_NOT_SUPPORTED;

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(device);
    NU_USB_PTRCHK(pipe);
    NU_USB_PTRCHK(stack);

    /* Invoke the dispatch table worker function. */
    if (((NU_USBF_DRVR_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
        New_Transfer != NU_NULL)
    {
        status =
            ((NU_USBF_DRVR_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
            New_Transfer (cb, stack, device, pipe);
    }

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_DRVR_Notify
*
* DESCRIPTION
*
*       Invokes the notify worker function for the supplied class
*       driver.
*
* INPUTS
*
*       cb                                  Class driver for which this
*                                           callback is meant for.
*       stack                               Stack invoking this function.
*       device                              Device on which this event has
*                                           happened.
*       event                               USB event that has occurred.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the class driver
*                                           could process the event
*                                           successfully.
*       NU_USB_INVLD_ARG                    Indicates the one of the input
*                                           parameters has been incorrect
*                                           AND/OR the event could not be
*                                           processed without an error.
*
**************************************************************************/
STATUS  NU_USBF_DRVR_Notify (NU_USB_DRVR   *cb,
                             NU_USB_STACK  *stack,
                             NU_USB_DEVICE *device,
                             UINT32         event)
{
    STATUS  status = NU_USB_NOT_SUPPORTED;
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(device);
    NU_USB_PTRCHK(stack);

    /* Invoke the dispatch table worker function. */
    if (((NU_USBF_DRVR_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
        Notify != NU_NULL)
    {
        status =
            ((NU_USBF_DRVR_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
            Notify (cb, stack, device, event);
    }

    return (status);
}

/**************************************************************************
*
* FUNCTION
*
*       _NU_USBF_DRVR_Delete
*
* DESCRIPTION
*
*       This function un-initializes the given class driver
*       Since the base class driver doesn't have any specific
*       un-initialization requirements, it invokes the usb_delete function
*
* INPUTS
*
*       drvr                                Class driver to be
*                                           un-initialized.
*
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the class driver
*                                           has been uninitialized
*                                           successfully.
*       NU_USB_INVLD_ARG                    Indicates that the supplied
*                                           parameter is incorrect.
*
**************************************************************************/
STATUS  _NU_USBF_DRVR_Delete (VOID *cb)
{

    NU_USB_PTRCHK(cb);

    return (_NU_USB_DRVR_Delete (cb));
}

/* Following functions should only be visible when stack is configured
 * for USB 3.0. */
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

/**************************************************************************
*
* FUNCTION
*
*       NU_USBF_DRVR_Get_Status
*
* DESCRIPTION
*
*       This function is called by Nucleus USB function stack when a 
*       Get_Status request is recieved and recipient of this requst is 
*       interface.
*       Dispatch table function called in this function must return 
*       interface status as a 16-bit value encoded as described in USB 
*       specifications.
*
* INPUTS
*
*       cb                                  Pointer to NU_USB_DRVR control
*                                           block.
*       status_out                          Output variable containing 
*                                           16-bit status value of 
*                                           interface encoded according 
*                                           to USB specifications when the 
*                                           function returns.
*
* OUTPUTS
*
*       NU_USB_INVLD_ARG                    Indicates that the supplied
*                                           parameter is incorrect.
*
**************************************************************************/
STATUS NU_USBF_DRVR_Get_Status(NU_USB_DRVR *cb,
                                UINT16       *status_out)
{
    STATUS status = NU_USB_INVLD_ARG;
    
    if ( cb != NU_NULL && status_out != NU_NULL)
    {
        if ( (((NU_USB *) cb)->usb_dispatch) != NU_NULL )
        {
            if (((NU_USBF_DRVR_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
                    Get_Status != NU_NULL)
            {
                status =
                    ((NU_USBF_DRVR_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
                        Get_Status (cb, status_out);
            }
        }
    }
    
    return ( status );
}

#endif /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */
#endif /* _USBF_DRVR_EXT_C.*/
/* ======================  End Of File  ================================ */

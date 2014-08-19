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
*        nu_usb_stack_ext.c
*
* COMPONENT
*
*        Nucleus USB Software : Stack component.
*
* DESCRIPTION
*       This file provides the implementation of services exported by base
*       stack component of Nucleus USB software.
*
*
* DATA STRUCTURES
*       None
*
* FUNCTIONS
*       NU_USB_STACK_Remove_Hw               Removes a HW controller from
*                                            stack.
*       NU_USB_STACK_Add_Hw                  Adds a HW controller to the
*                                            stack.
*       NU_USB_STACK_Deregister_Drvr         Deregisters a class driver
*                                            from stack.
*       NU_USB_STACK_Flush_Pipe              Flushes a pipe.
*       NU_USB_STACK_Get_Config              Get config from device.
*       NU_USB_STACK_Get_Dev_Status          Get status from device.
*       NU_USB_STACK_Get_Endpoint_Status     Get status from endpoint.
*       NU_USB_STACK_Get_Interface           Get Active alternate setting.
*       NU_USB_STACK_Get_Interface_Status    Get status from interface.
*       NU_USB_STACK_Is_Endpoint_Stalled     Find endpoint stall status.
*       NU_USB_STACK_Is_Valid_Device         Checks validity of device
*                                            control block.
*       NU_USB_STACK_Lock                    Grabs stack lock.
*       NU_USB_STACK_Register_Drvr           Registers class driver to
*                                            stack.
*       NU_USB_STACK_Set_Config              Set Active configuration.
*       NU_USB_STACK_Set_Device_Status       Get status from device.
*       NU_USB_STACK_Set_Intf                set active alternate setting.
*       NU_USB_STACK_Stall_Endpoint          Stalls an endpoint
*       NU_USB_STACK_Unlock                  Releases stack lock.
*       NU_USB_STACK_Unstall                 clears stall on an endpoint.
*       NU_USB_Submit_IRP                    Submit an IRP on a pipe.
*       NU_USB_Cancel_IRP                    Cancel an IRP on a pipe.
*       _NU_USB_STACK_Create                 Create an instance of stack.
*       _NU_USB_STACK_Delete                 Delete an instance of stack.
*       _NU_USB_STACK_Deregister_Drvr        Deregisters a class driver
*                                            from stack.
*       _NU_USB_STACK_Get_Config             Get active configuration from
*                                            a device.
*       _NU_USB_STACK_Get_Intf               Get active alt setting from an
*                                            interface
*       _NU_USB_STACK_Get_Intf_Status        returns the status of an
*                                            interface.
*       _NU_USB_STACK_Is_Valid_Device        Checks the validity of device
*                                            control
*                                            block
*       _NU_USB_STACK_Lock                   Locks the access to critical
*                                            data structures.
*       _NU_USB_STACK_Register_Drvr          registers a class driver to
*                                            stack.
*       _NU_USB_STACK_Unlock                 Releases the lock.
*       NU_USB_STACK_Function_Suspend        This function is called to 
*                                            send a selective suspend 
*                                            (function suspend) command 
*                                            to a super speed device.
*
* DEPENDENCIES
*
*       nu_usb.h                             All USB definitions
*
************************************************************************/
#ifndef USB_STACK_EXT_C
#define USB_STACK_EXT_C

/* ======================  Include Files  ============================= */
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "connectivity/nu_usb.h"
/***********************************************************************/

/*************************************************************************
* FUNCTION
*               _NU_USB_STACK_Create
*
* DESCRIPTION
*      This function creates an instance of the stack. This is a protected
*      API which is meant to be used by stack extenders, by providing their
*      own dispatch table. Overriding/Extending/Reusing the base behavior.
*
* INPUTS
*       cb          pointer to stack control block.
*       subsys      pointer to subsystem control block this stack should be
*                   placed.
*       name        Name for this instance.
*       dispatch    Pointer to a variable to hold the pointers to dispatch
*                   table functions.
*
* OUTPUTS
*       NU_SUCCESS       Indicated successful completion.
*       NU_USB_INVLD_ARG Indicates that one or more arguments passed to 
*                        this function are invalid.
*
*************************************************************************/
STATUS _NU_USB_STACK_Create (NU_USB_STACK * cb,
                             NU_USB_SUBSYS * subsys,
                             CHAR * name,
                             const VOID *dispatch)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(subsys);
    NU_USB_PTRCHK_RETURN(name);
    NU_USB_PTRCHK_RETURN(dispatch);

    /* Create base. */
    status = _NU_USB_Create ((NU_USB *) cb, subsys, name, dispatch);

    /* Initialize control block. */
    cb->otg_status_func = NU_NULL;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               _NU_USB_STACK_Delete
*
* DESCRIPTION
*       This function deletes a specified stack.
*
* INPUTS
*       cb      pointer to stack control block.
*
* OUTPUTS
*       NU_SUCCESS          Stack deleted successfully
*       NU_USB_INVLD_ARG    Indicates that one or more arguments passed to 
*                           this function are invalid.
*
*************************************************************************/
STATUS _NU_USB_STACK_Delete (VOID *cb)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);

    status = _NU_USB_Delete (cb);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_STACK_Register_Drvr
*
* DESCRIPTION
*       This function registers new class drivers to the stack.
*
* INPUTS
*       cb              pointer to stack control block
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
STATUS NU_USB_STACK_Register_Drvr (NU_USB_STACK * cb,
                                   NU_USB_DRVR * class_driver)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(class_driver);
    NU_USB_PTRCHK_RETURN(((NU_USB *) cb)->usb_dispatch);

   status = ((NU_USB_STACK_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
            Register_Class_Driver (cb, class_driver);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_STACK_Deregister_Drvr
*
* DESCRIPTION
*       This function deregisters a class drivers from the stack.
*
* INPUTS
*       cb              pointer to stack control block
*       class_driver    pointer to class driver control block to be
*                       deregistered.
*
* OUTPUTS
*   NU_SUCCESS              Indicates that the driver has been successfully
*                           deregistered.
*   NU_USB_INVLD_ARG        Indicates that one or more arguments passed to this
*                           function are invalid.
*
*************************************************************************/
STATUS NU_USB_STACK_Deregister_Drvr (NU_USB_STACK * cb,
                                     NU_USB_DRVR * class_driver)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(class_driver);
    NU_USB_PTRCHK_RETURN(((NU_USB *) cb)->usb_dispatch);

    status = ((NU_USB_STACK_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
            Deregister_Class_Driver (cb, class_driver);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_STACK_Add_Hw
*
* DESCRIPTION
*       This function adds a new Hardware controller to the stack. The
*       controller is made operational. In case of Host stack root hub
*       contained in the controller is enumerated.
*
* INPUTS
*
*       cb          pointer to the stack control block.
*       controller  pointer to the h/w control block.
*
* OUTPUTS
*
*   NU_SUCCESS              Indicates that the driver has been successfully
*                           registered.
*   NU_USB_INVLD_ARG        Indicates that one or more arguments passed to this
*                           function are invalid.
*
*************************************************************************/
STATUS NU_USB_STACK_Add_Hw (NU_USB_STACK * cb,
                            NU_USB_HW * controller)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(controller);
    NU_USB_PTRCHK_RETURN(((NU_USB *) cb)->usb_dispatch);

    status = ((NU_USB_STACK_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
               Add_Controller (cb, controller);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_STACK_Remove_Hw
*
* DESCRIPTION
*     Removes a controller hardware that was earlier added to the stack.
*     In case of host stack the root hub is de-enumerated causing the whole
*     associated device topology to be de-enumerated.
*
* INPUTS
*       cb          pointer to the stack control block.
*       controller  pointer to the h/w control block.
*
* OUTPUTS
*       NU_SUCCESS       Indicates successful completion
*       NU_USB_INVLD_ARG Indicates that one or more arguments passed to 
*                        this function are invalid.
*
*************************************************************************/
STATUS NU_USB_STACK_Remove_Hw (NU_USB_STACK * cb,
                               NU_USB_HW * controller)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(controller);
    NU_USB_PTRCHK_RETURN(((NU_USB *) cb)->usb_dispatch);

    status = ((NU_USB_STACK_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
            Remove_Controller (cb, controller);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_STACK_Get_Dev_Status
*
* DESCRIPTION
*       This function returns the current status of device on a stack.
*
* INPUTS
*   cb          pointer to stack control block
*   device      pointer to device control block
*   status_out  pointer to a variable to hold returned status of device.
*
* OUTPUTS
*   NU_SUCCESS        Indicates successful completion.
*   NU_USB_INVLD_ARG  Indicates that one or more arguments passed to 
*                     this function are invalid.
*
*************************************************************************/
STATUS NU_USB_STACK_Get_Dev_Status (NU_USB_STACK * cb,
                                    NU_USB_DEVICE * device,
                                    UINT16 *status_out)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(device);
    NU_USB_PTRCHK_RETURN(status_out);
    NU_USB_PTRCHK_RETURN(((NU_USB *) cb)->usb_dispatch);

    status = (((NU_USB_STACK_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
            Get_Device_Status (cb, device, status_out));

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_STACK_Set_Device_Status
*
* DESCRIPTION
*       This function sets the status of the specified device. Status
*       supplied contains a bit map defined by USB standard.
*
* INPUTS
*   cb          pointer to stack control block.
*   device      pointer to device control block.
*   status      status to be applied to device.
*
* OUTPUTS
*   NU_SUCCESS          Indicates successful completion.
*   NU_USB_INVLD_ARG    Indicates that one or more arguments passed to 
*                       this function are invalid.
*
*************************************************************************/
STATUS NU_USB_STACK_Set_Device_Status (NU_USB_STACK * cb,
                                       NU_USB_DEVICE * device,
                                       UINT16 dev_status)
{
    STATUS status = NU_USB_INVLD_ARG;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(device);
    NU_USB_PTRCHK_RETURN(((NU_USB *) cb)->usb_dispatch);

    if (((NU_USB_STACK_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
        Set_Device_Status)
        status = ((NU_USB_STACK_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
                Set_Device_Status (cb, device, dev_status);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_STACK_Get_Config
*
* DESCRIPTION
*       Gets the bConfigurationNumber of the currently active configuration
*  of the device. 0 indicates that it's un-configured.
*
* INPUTS
*   cb          pointer to stack control block.
*   device      pointer to device control block.
*   cnfgno_out  pointer to a variable to hold bConfigurationValue of the
*               active configuration on the device.A value of 0 indicates
*               that the device is un-configured.
*
* OUTPUTS
*   NU_SUCCESS              Indicates successful completion of the service
*   NU_USB_INVLD_ARG        Indicates that one or more arguments passed to this
*                           function are invalid.
*
*************************************************************************/
STATUS NU_USB_STACK_Get_Config (NU_USB_STACK * cb,
                                NU_USB_DEVICE * device,
                                UINT8 *cnfgno_out)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(device);
    NU_USB_PTRCHK_RETURN(cnfgno_out);
    NU_USB_PTRCHK_RETURN(((NU_USB *) cb)->usb_dispatch);

    status = ((NU_USB_STACK_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
            Get_Configuration (cb, device, cnfgno_out);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_STACK_Set_Config
*
* DESCRIPTION
*    Set the configuration to the specified bConfigurationValue. If the
* specified value is 0, the device is un-configured. This API is meant
* to be used only by device drivers and not by interface drivers.
*
* INPUTS
*   cb          pointer to stack control block.
*   device      pointer to device control block.
*   cnfgno      bConfigurationValue of the config descriptors that needs to
*               be made active on the device. A value of 0 indicates that the
*               device should be un-configured.
* OUTPUTS
*   NU_SUCCESS              Indicates successful completion of the service
*   NU_USB_INVLD_ARG        Indicates that one or more arguments passed to 
*                           this function are invalid.
*
*************************************************************************/
STATUS NU_USB_STACK_Set_Config (NU_USB_STACK * cb,
                                NU_USB_DEVICE * device,
                                UINT8 cnfgno)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(device);
    NU_USB_PTRCHK_RETURN(((NU_USB *) cb)->usb_dispatch);

    status = NU_USB_INVLD_ARG;

    if (((NU_USB_STACK_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
        Set_Configuration)
        status = ((NU_USB_STACK_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
                Set_Configuration (cb, device, cnfgno);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_STACK_Get_Interface
*
* DESCRIPTION
*  gets the currently active alternate setting on the specified interface
*  of the device.
*
*
* INPUTS
*   cb                      pointer to stack control block.
*   device                  pointer to device control block.
*   interface_index         interface number on the specified device
*                           in the  range of 0 to NU_USB_MAX_INTERFACES-1
*   alt_setting_index_out   pointer to a variable to contain
*                           bAlternateSetting value for the interface
*                           in the range of 0 to NU_USB_MAX_ALT_SETTINGS-1
*
* OUTPUTS
*   NU_SUCCESS              Indicates successful completion of the service
*   NU_USB_INVLD_ARG        Indicates that one or more arguments passed to 
*                           this function are invalid.
*
*************************************************************************/
STATUS NU_USB_STACK_Get_Interface (NU_USB_STACK * cb,
                                   NU_USB_DEVICE * device,
                                   UINT8 interface_index,
                                   UINT8 *alt_setting_index_out)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(device);
    NU_USB_PTRCHK_RETURN(alt_setting_index_out);
    NU_USB_PTRCHK_RETURN(((NU_USB *) cb)->usb_dispatch);

    status = ((NU_USB_STACK_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
            Get_Interface (cb, device, interface_index, alt_setting_index_out);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_STACK_Set_Intf
*
* DESCRIPTION
*    Set the specified interface to the specified bAlternateSetting.
* This API if used by interface drivers, they should restrict
* to their own interface of the device. A device driver doesn't have
* have this restriction. The interface number and alternate setting
* number refer to the currently active configuration on the device.
*
* INPUTS
*   cb                  pointer to stack control block.
*   device              pointer to device control block.
*   interface_index     interface number on the specified device
*                       in the  range of 0 to NU_USB_MAX_INTERFACES-1
*   alt_setting_index   bAlternateSetting value for the interface
*                       in the range of 0 to NU_USB_MAX_ALT_SETTINGS-1
* OUTPUTS
*   NU_SUCCESS              Indicates successful completion of the service
*   NU_USB_INVLD_ARG        Indicates that one or more arguments passed to 
*                           this function are invalid.
*
*************************************************************************/
STATUS NU_USB_STACK_Set_Intf (NU_USB_STACK * cb,
                              NU_USB_DEVICE * device,
                              UINT8 interface_index,
                              UINT8 alt_setting_index)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(device);
    NU_USB_PTRCHK_RETURN(((NU_USB *) cb)->usb_dispatch);

    status = ((NU_USB_STACK_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
            Set_Interface (cb, device, interface_index, alt_setting_index);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_STACK_Get_Interface_Status
*
* DESCRIPTION
*   Gets the status of the specified interface on a device.
*
* INPUTS
*   cb              pointer to stack control block.
*   dev             pointer to device control block.
*   interface_index interface number on the specified device
*                   in the  range of 0 to USB_MAX_INTERFACES-1
*   status          pointer to variable to store returned interface status.
*                   Interface status is a bit map defined by the USB
*                   standard.
*
* OUTPUTS
*   NU_SUCCESS              Indicates successful completion of the service
*   NU_USB_INVLD_ARG        Indicates that one or more arguments passed to 
*                           this function are invalid.
*
*************************************************************************/
STATUS NU_USB_STACK_Get_Interface_Status (NU_USB_STACK * cb,
                                          NU_USB_DEVICE * device,
                                          UINT8 interface_index,
                                          UINT16 *interface_status)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(device);
    NU_USB_PTRCHK_RETURN(interface_status);
    NU_USB_PTRCHK_RETURN(((NU_USB *) cb)->usb_dispatch);

    status = ((NU_USB_STACK_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
            Get_Interface_Status (cb, device, interface_index,
                                  interface_status);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_STACK_Stall_Endpoint
*
* DESCRIPTION
*       This function sets the halted feature of the endpoints
*       associated with the given pipe.
*
* INPUTS
*       cb      pointer  to the stack control block.
*       pipe    pointer  to the pipe control block that identifies the endpoint.
*
* OUTPUTS
*   NU_SUCCESS              Indicates successful completion of the service
*   NU_USB_INVLD_ARG        Indicates that one or more arguments passed to this
*                           function are invalid.
*
*************************************************************************/
STATUS NU_USB_STACK_Stall_Endpoint (NU_USB_STACK * cb,
                                    NU_USB_PIPE * pipe)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(pipe);
    NU_USB_PTRCHK_RETURN(((NU_USB *) cb)->usb_dispatch);

    status = ((NU_USB_STACK_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
               Stall_Endpoint (cb, pipe);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_STACK_Unstall
*
* DESCRIPTION
*       This function clears the halted feature of the endpoints
*       associated with the given pipe.
*
* INPUTS
*       cb      pointer  to the stack control block.
*       pipe    pointer  to the pipe control block that identifies the endpoint.
*
* OUTPUTS
*   NU_SUCCESS              Indicates successful completion of the service
*   NU_USB_INVLD_ARG        Indicates that one or more arguments passed to 
*                           this function are invalid.
*
*************************************************************************/
STATUS NU_USB_STACK_Unstall (NU_USB_STACK * cb,
                             NU_USB_PIPE * pipe)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Validate the parameters as this function
       is critical and real time */
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(pipe);
    NU_USB_PTRCHK_RETURN(((NU_USB *) cb)->usb_dispatch);

    status = ((NU_USB_STACK_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
            Unstall_Endpoint (cb, pipe);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_STACK_Get_Endpoint_Status
*
* DESCRIPTION
*   Gets the status of the specified endpoint.
*
*
* INPUTS
*       cb      pointer  to the stack control block.
*       pipe    pointer  to the pipe control block that identifies the endpoint.
*       status  pointer to a variable to store returned endpoint status.
*
*
* OUTPUTS
*   NU_SUCCESS              Indicates successful completion of the service
*   NU_USB_INVLD_ARG        Indicates that one or more arguments passed to this
*                           function are invalid.
*
*************************************************************************/
STATUS NU_USB_STACK_Get_Endpoint_Status (NU_USB_STACK * cb,
                                         NU_USB_PIPE * pipe,
                                         UINT16 *ep_status)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(pipe);
    NU_USB_PTRCHK_RETURN(ep_status);
    NU_USB_PTRCHK_RETURN(((NU_USB *) cb)->usb_dispatch);

    status = (((NU_USB_STACK_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
            Get_Endpoint_Status (cb, pipe, ep_status));

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_STACK_Is_Endpoint_Stalled
*
* DESCRIPTION
*  Gets the endpoint status to find out if the endpoint is stalled.
*
* INPUTS
*   cb      pointer  to the stack control block.
*   pipe    pointer  to the pipe control block that identifies the endpoint.
*   status  pointer to a variable to store returned endpoint status.
*
* OUTPUTS
*       NU_SUCCESS              Indicates successful completion of the service
*       NU_USB_INVLD_ARG        Indicates that one or more arguments passed to
*                               this function are invalid.
*
*************************************************************************/
STATUS NU_USB_STACK_Is_Endpoint_Stalled (NU_USB_STACK * cb,
                                         NU_USB_PIPE * pipe,
                                         DATA_ELEMENT * ep_status)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(pipe);
    NU_USB_PTRCHK_RETURN(ep_status);
    NU_USB_PTRCHK_RETURN(((NU_USB *) cb)->usb_dispatch);

    status = ((NU_USB_STACK_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
            Is_Endpoint_Stalled (cb, pipe, ep_status);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_Submit_IRP
*
* DESCRIPTION
*       This function is used to submit an IRP on a given pipe on the stack.
*
* INPUTS
*       cb      pointer to stack control block
*       irp     pointer to IRP control block to be submitted.
*       pipe    pointer to pipe control block on which irp is submitted.
*
* OUTPUTS
*       NU_SUCCESS          indicates successful completion.
*       NU_USB_INVLD_ARG    indicates one or more arguments are invalid.
*
*************************************************************************/
STATUS NU_USB_Submit_IRP (NU_USB_STACK * cb,
                          NU_USB_IRP * irp,
                          NU_USB_PIPE * pipe)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Validate the parameters as this function
       is critical and real time */
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(irp);
    NU_USB_PTRCHK_RETURN(pipe);
    NU_USB_PTRCHK_RETURN(((NU_USB *) cb)->usb_dispatch);

    if ( pipe->device->state & USB_STATE_SUSPENDED)
    {
        status = NU_USB_DEVICE_INVLD_STATE;
    }
    else
    {
        irp->pipe = pipe;
            status = ((NU_USB_STACK_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
                 Submit_IRP (cb, irp, pipe);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_Cancel_IRP
*
* DESCRIPTION
*       This function is used to cancel an IRP on a given pipe on the stack.
*
* INPUTS
*       cb      pointer to stack control block
*       pipe    pointer to pipe control block on which irp is being cancelled.
*
* OUTPUTS
*       NU_SUCCESS          indicates successful completion.
*       NU_USB_INVLD_ARG    indicates one or more arguments are invalid.
*
*************************************************************************/
STATUS NU_USB_Cancel_IRP (NU_USB_STACK * cb,
                          NU_USB_PIPE * pipe)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Validate the parameters as this function
       is critical and real time */
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(pipe);
    NU_USB_PTRCHK_RETURN(((NU_USB *) cb)->usb_dispatch);

    status = ((NU_USB_STACK_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
                  Flush_Pipe (cb, pipe);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_STACK_Flush_Pipe
*
* DESCRIPTION
*       This function is used to flush a given pipe on the stack. All the
*       pending IRP's on the pipe would be flushed after this call.
*
* INPUTS
*       cb      pointer to stack control block.
*       pipe    pointer to pipe control block to be flushed.
*
* OUTPUTS
*       NU_SUCCESS          indicates successful completion.
*       NU_USB_INVLD_ARG    indicates one or more arguments are invalid.
*
*************************************************************************/
STATUS NU_USB_STACK_Flush_Pipe (NU_USB_STACK * cb,
                                NU_USB_PIPE * pipe)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(pipe);
    NU_USB_PTRCHK_RETURN(((NU_USB *) cb)->usb_dispatch);

    status = ((NU_USB_STACK_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
             Flush_Pipe (cb, pipe);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_STACK_Lock
*
* DESCRIPTION
*       Grabs the stack lock.
*
* INPUTS
*       cb          pointer to stack control block.
*
* OUTPUTS
*       NU_SUCCESS      Indicates successful completion.
*
*************************************************************************/
STATUS NU_USB_STACK_Lock (NU_USB_STACK * cb)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(((NU_USB *) cb)->usb_dispatch);

    status = ((NU_USB_STACK_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
             Lock (cb);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_STACK_Unlock
*
* DESCRIPTION
*       Releases the stack lock.
*
* INPUTS
*       cb          pointer to stack control block.
*
* OUTPUTS
*       NU_SUCCESS        Indicates successful completion.
*       NU_USB_INVLD_ARG  Indicates that one or more arguments passed to 
*                         this function are invalid.
*
*************************************************************************/
STATUS NU_USB_STACK_Unlock (NU_USB_STACK * cb)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(((NU_USB *) cb)->usb_dispatch);

    status = (((NU_USB_STACK_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
            Unlock (cb));

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_STACK_Is_Valid_Device
*
* DESCRIPTION
*      NU_USB_DEVICE pointer is used as an handle by class drivers for
*      using the services of stack. This function checks if the device
*      pointer is still valid.
*
* INPUTS
*       cb      pointer to stack control block.
*       device  pointer to device control block to be validated.
*
* OUTPUTS
*       NU_TRUE     Device pointer is valid.
*       NU_FALSE    Device pointer is invalid.
*
*************************************************************************/
BOOLEAN NU_USB_STACK_Is_Valid_Device (NU_USB_STACK * cb,
                                      NU_USB_DEVICE * device)
{
    BOOLEAN status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    if((cb == NU_NULL)||
       (device == NU_NULL)||
       (((NU_USB *) cb)->usb_dispatch == NU_NULL))
    {

        NU_USER_MODE();
        return (0x00);
    }

    status = (((NU_USB_STACK_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
            Is_Valid_Device (cb, device));

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_STACK_Start_Session
*
* DESCRIPTION
*      This function will start a session for 'otg' devices
*
* INPUTS
*       cb      pointer to stack control block.
*       hw      pointer to HW control block
*       port_id Port identifier for the port on which the OTG device
*               is connected.
*       delay   time for which starting a session should be attempted
*               in milliseconds.
*
* OUTPUTS
*       NU_SUCCESS        On success when a session is started.
*       NU_USB_INVLD_ARG  Indicates that one or more arguments passed to 
*                         this function are invalid.
*
*************************************************************************/
STATUS NU_USB_STACK_Start_Session (NU_USB_STACK * cb, NU_USB_HW * hw,
                                   UINT8 port_id, UINT16 delay)

{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(hw);
    NU_USB_PTRCHK_RETURN(((NU_USB *) cb)->usb_dispatch);

    status = ((NU_USB_STACK_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
             Start_Session (cb, hw, port_id, delay);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               NU_USB_STACK_End_Session
*
* DESCRIPTION
*      This function will end a session for 'otg' devices
*
* INPUTS
*       cb      pointer to stack control block.
*       hw      pointer to HW control block
*       port_id Port identifier for the port on which the OTG device
*               is connected.
*
* OUTPUTS
*       NU_SUCCESS  on success when a session is ended.
*       NU_USB_INVLD_ARG  Indicates that one or more arguments passed to 
*                         this function are invalid.
*
*************************************************************************/
STATUS NU_USB_STACK_End_Session (NU_USB_STACK * cb,
                        NU_USB_HW * hw,
                        UINT8 port_id)

{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(hw);
    NU_USB_PTRCHK_RETURN(((NU_USB *) cb)->usb_dispatch);

    status = ((NU_USB_STACK_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
             End_Session (cb, hw, port_id);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               _NU_USB_STACK_Register_Drvr
*
* DESCRIPTION
*       This function registers a class driver to the stack. Registration
*       is successful only if number of class drivers registered didn't
*       exceed maximum number of class drivers that the stack can support.
*
* INPUTS
*       stack           pointer to stack control block
*       class_driver    pointer to class driver control block being registered
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion
*       NU_USB_MAX_EXCEEDED Indicates stack has already registered maximum
*                           number of class drivers possible.
*       NU_USB_INVLD_ARG    Indicates that one or more arguments passed to 
*                           this function are invalid.
*
*************************************************************************/
STATUS _NU_USB_STACK_Register_Drvr (NU_USB_STACK * stack,
                                    NU_USB_DRVR * class_driver)
{
    UINT16 i;
    UINT8 score;
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(stack);
    NU_USB_PTRCHK_RETURN(class_driver);

    /* Find the total number of class drivers already
     * registered with stack.
     */
    i = (UINT16) stack->num_class_drivers;

    if (i == NU_USB_MAX_CLASS_DRIVERS)
    {
        status = NU_USB_MAX_EXCEEDED;
    }
    else
    {
        stack->class_drivers[i].driver = class_driver;

        stack->num_class_drivers++;

        status = NU_USB_DRVR_Get_Score (class_driver, &score);

        stack->class_drivers[i].list_node.cs_priority = score;

        NU_Priority_Place_On_List ((CS_NODE **) & (stack->class_driver_list_head),
                                   (CS_NODE *) & (stack->class_drivers[i]));
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/************************************************************************
 *
 * FUNCTION
 *     _NU_USB_STACK_Deregister_Drvr
 *
 * DESCRIPTION
 *   It deregisters a USB driver that has been earlier registered using
 * NU_USB_Register_Driver. The de-registration is successful, only if no
 * device/interface is currently attached to the driver.
 *
 * INPUTS
 *     NU_USB_DRIVER *drvr The same driver specification that was earlier
 *            passed to NU_USB_Register_Driver.
 *
 * OUTPUTS
 *    NU_SUCCESS            Indicates that the driver has been successfully
 *                          deregistered.
 *    NU_USB_INVLD_ARG      Indicates that the drvr passed is NU_NULL or
 *                          that the drvr is not a known driver to USB.
 *
 *************************************************************************/
STATUS _NU_USB_STACK_Deregister_Drvr (NU_USB_STACK * stack,
                                      NU_USB_DRVR * class_driver)
{
    UINT16 i;
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(stack);
    NU_USB_PTRCHK_RETURN(class_driver);

    for (i = 0; i < stack->num_class_drivers; i++)
        if (stack->class_drivers[i].driver == class_driver)
            break;

    if (i == stack->num_class_drivers)
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        --stack->num_class_drivers;

        NU_Remove_From_List ((CS_NODE **) & (stack->class_driver_list_head),
                             (CS_NODE *) & (stack->class_drivers[i]));

        if (stack->num_class_drivers != i)
        {
            /* Copy the last class driver in the vacant position. */
            stack->class_drivers[i] =
                stack->class_drivers[stack->num_class_drivers];

            /* Now remove the last class driver from the list as well. */
            NU_Remove_From_List ((CS_NODE **) & (stack->class_driver_list_head),
                                 (CS_NODE *) & (stack->class_drivers[stack->num_class_drivers]));

            /* Set the last driver NU_NULL. */
            stack->class_drivers[stack->num_class_drivers].driver = NU_NULL;

            /* Now place back the element in the list. */
            NU_Priority_Place_On_List ((CS_NODE **) & (stack->class_driver_list_head),
                                       (CS_NODE *) & (stack->class_drivers[i]));
        }

        status = NU_SUCCESS;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               _NU_USB_STACK_Get_Config
*
* DESCRIPTION
*       This function returns active configuration number on a device.
*
* INPUTS
*       stack       pointer to stack control block.
*       device      pointer to device control block.
*       cnfgno_out  pointer to a location to hold output configuration
*                   number
*
* OUTPUTS
*       NU_SUCCESS        Indicates successful completion
*       NU_USB_INVLD_ARG  Indicates that one or more arguments passed to 
*                         this function are invalid.
*
*************************************************************************/
STATUS _NU_USB_STACK_Get_Config (NU_USB_STACK * stack,
                                 NU_USB_DEVICE * device,
                                 UINT8 *cnfgno_out)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(stack);
    NU_USB_PTRCHK_RETURN(device);
    NU_USB_PTRCHK_RETURN(cnfgno_out);

    *cnfgno_out =
        (device->active_cnfg_num ==
         NU_USB_MAX_CONFIGURATIONS) ? (UINT8)0 : device->active_cnfg_num;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               _NU_USB_STACK_Get_Intf
*
* DESCRIPTION
*       This function returns the active alternate setting index on a given
*       device's given interface.
*
* INPUTS
*       stack                   pointer to stack's control block.
*       device                  pointer to device control block.
*       interface_index         Index of interface on the device.
*       alt_setting_index_out   Pointer to a location to hold returned
*                               active alternate setting's index.
*
* OUTPUTS
*       NU_SUCCESS        Indicates successful completion.
*       NU_USB_INVLD_ARG  Indicates that one or more arguments passed to 
*                         this function are invalid.
*
*************************************************************************/
STATUS _NU_USB_STACK_Get_Intf (NU_USB_STACK * stack,
                               NU_USB_DEVICE * device,
                               UINT8 interface_index,
                               UINT8 *alt_setting_index_out)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(stack);
    NU_USB_PTRCHK_RETURN(device);
    NU_USB_PTRCHK_RETURN(alt_setting_index_out);

    /* Remove Lint warning. */
    NU_UNUSED_PARAM(stack);
    *alt_setting_index_out =
        device->config_descriptors[device->active_cnfg_num]->
        intf[interface_index].current->desc->bAlternateSetting;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               _NU_USB_STACK_Get_Intf_Status
*
* DESCRIPTION
*       This function is used to retrieve the status of an Interface on the
*       specified device and interface index. This function is provided for
*       future extension as Interface status is currently reserved by USB
*       Specification and always returns zero.
*
* INPUTS
*       stack       pointer to stack control block.
*       device      pointer to concerned device's control block.
*       intf_index  Index of concerned interface on device.
*       status      pointer to a location to hold returned status.
*
* OUTPUTS
*       NU_SUCCESS        Indicates successful completion.
*       NU_USB_INVLD_ARG  Indicates that one or more arguments passed to 
*                         this function are invalid.
*
*************************************************************************/
STATUS _NU_USB_STACK_Get_Intf_Status (NU_USB_STACK * stack,
                                      NU_USB_DEVICE * device,
                                      UINT8 intf_index,
                                      UINT16 *status)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(stack);
    NU_USB_PTRCHK_RETURN(device);
    NU_USB_PTRCHK_RETURN(status);

    /* Remove Lint warning. */
    /* Status for interface is reserved in the USB spec */
    *status = 0;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               _NU_USB_STACK_Lock
*
* DESCRIPTION
*       This is a protected interface meant to be used by extenders of this
*       component. This function can be used to protect the critical
*       resources of the subsystem
*
* INPUTS
*       cb          pointer to stack control block.
*
* OUTPUTS
*       NU_SUCCESS        Indicates successful completion.
*       NU_USB_INVLD_ARG  Indicates that one or more arguments passed to 
*                         this function are invalid.
*
*************************************************************************/
STATUS _NU_USB_STACK_Lock (NU_USB_STACK * cb)
{
    NU_USB_PTRCHK(cb);

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               _NU_USB_STACK_Unlock
*
* DESCRIPTION
*       This is a protected interface meant to be used by extenders of this
*       component. This function can be used to unlock the critical
*       resources locked by _NU_USB_STACK_Lock.
*
* INPUTS
*       cb          pointer to stack control block.
*
* OUTPUTS
*       NU_SUCCESS        Indicates successful completion.
*       NU_USB_INVLD_ARG  Indicates that one or more arguments passed to 
*                         this function are invalid.
*
*************************************************************************/
STATUS _NU_USB_STACK_Unlock (NU_USB_STACK * cb)
{
    NU_USB_PTRCHK(cb);

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               _NU_USB_STACK_Is_Valid_Device
*
* DESCRIPTION
*      NU_USB_DEVICE pointer is used as an handle by class drivers for
*      using the services of stack. This function checks if the device
*      pointer is still valid.
*
* INPUTS
*       cb      pointer to stack control block.
*       device  pointer to device control block to be validated.
*
* OUTPUTS
*       NU_TRUE           Device pointer is valid.
*       NU_USB_INVLD_ARG  Indicates that one or more arguments passed to 
*                         this function are invalid.
*
*************************************************************************/
BOOLEAN _NU_USB_STACK_Is_Valid_Device (NU_USB_STACK * cb,
                                       NU_USB_DEVICE * device)
{
    /* Remove Lint warning. */
    NU_UNUSED_PARAM(cb);
    NU_UNUSED_PARAM(device);

    return NU_TRUE;
}

/*************************************************************************
* FUNCTION
*               _NU_USB_STACK_Start_Session
*
* DESCRIPTION
*      This function will start a session for 'otg' devices
*
* INPUTS
*       cb      pointer to stack control block.
*       hw      pointer to HW control block
*       port_id Port identifier for the port on which the OTG device
*               is connected.
*
* OUTPUTS
*       NU_SUCCESS        On success when a session is started.
*       NU_USB_INVLD_ARG  Indicates that one or more arguments passed to 
*                         this function are invalid.
*
*************************************************************************/
STATUS _NU_USB_STACK_Start_Session (NU_USB_STACK * cb,
                        NU_USB_HW * hw,
                        UINT8 port_id, UINT16 delay)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(hw);

    /* Remove Lint warning. */
    NU_UNUSED_PARAM(cb);
    status = NU_USB_HW_Start_Session (hw, port_id, delay);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*               _NU_USB_STACK_End_Session
*
* DESCRIPTION
*      This function will end a session for 'otg' devices
*
* INPUTS
*       cb      pointer to stack control block.
*       hw      pointer to HW control block
*       port_id Port identifier for the port on which the OTG device
*               is connected.
*
* OUTPUTS
*       NU_SUCCESS        On success when a session is ended.
*       NU_USB_INVLD_ARG  Indicates that one or more arguments passed to
*                         this function are invalid.
*
*************************************************************************/
STATUS _NU_USB_STACK_End_Session (NU_USB_STACK * cb,
                        NU_USB_HW * hw,
                        UINT8 port_id)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(hw);

    /* Remove Lint warning. */
    NU_UNUSED_PARAM(cb);
    status = NU_USB_HW_End_Session (hw, port_id);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}
/*************************************************************************
* FUNCTION
*               NU_USB_STACK_OTG_Reg_Status
*
* DESCRIPTION
*      This function registers the call-back function for reporting OTG failures with stack.
*
* INPUTS
*      cb               Pointer to stack control block.
*      status_func      Pointer to call-back function for reporting OTG failures.
*
* OUTPUTS
*      NU_SUCCESS       Indicated successful completion.
*      NU_USB_INVLD_ARG Indicates that one or more arguments passed to
*                       this function are invalid.
*
*************************************************************************/
STATUS NU_USB_STACK_OTG_Reg_Status (NU_USB_STACK * cb,
                             NU_USB_STACK_OTG_STATUS_REPORT status_func)
{
    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);

    /* Save call-back function pointer in base stack. */
    cb->otg_status_func = status_func;

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_STACK_OTG_Report_Status
*
* DESCRIPTION
*      This function calls the OTG status call-back function with the given status.
*
* INPUTS
*      cb            Pointer to stack control block.
*      status_in     Status to be reported.
*
* OUTPUTS
*      NU_SUCCESS            Indicated successful completion.
*      NU_INVALID_FUNCTION   OTG status callback not registered with stack.
*      NU_USB_INVLD_ARG      Indicates that one or more arguments passed to 
*                            this function are invalid.
*
*************************************************************************/
STATUS NU_USB_STACK_OTG_Report_Status (NU_USB_STACK * cb,
                                       STATUS status_in)
{
    STATUS status_out;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);

    /* Check if Call-back function registered with the stack. */
    if(cb->otg_status_func  != NU_NULL)
    {
        /* Report given status. */
        cb->otg_status_func(status_in);

        /* Update return status. */
        status_out = NU_SUCCESS;
    }
    else
    {
        /* Report error to the caller. */
        status_out = NU_INVALID_FUNCTION;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status_out);
}

/*************************************************************************/

/* Following functions should only be visible when stack is configured
 * for Super Speed USB (USB 3.0). */

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_STACK_Function_Suspend
*
*   DESCRIPTION
*
*       User can call this function to handle a Set Feature
*       (Function_Suspend) standard request. This API can be called in both
*       host and function modes with appropriate arguments.
*
*   INPUTS
*
*       cb                  - Pointer to NU_USB_STACK control block.
*
*       intf                - Pointer to NU_USB_INTF control block.
*
*       func_suspend        - Boolean, containing value to be set for
*                             function suspend bit.
*                               NU_TRUE:    Set function suspend bit
*                                           position to 1.
*                               NU_FALSE:   Set function suspend bit
*                                           position to 0
*
*       rmt_wakeup          - Boolean, containing value to be set for
*                             remote wakeup bit.
*                               NU_TRUE:    Set remote wakeup bit
*                                           position to 1.
*                               NU_FALSE:   Set remote wakeup bit
*                                           position to 0.
*
*   OUTPUTS
*
*       NU_SUCCESS          - Operation Completed Successfully.
*
*       NU_USB_INVLD_ARG    - Indicates any of the input argument is
*                             invalid.
*
*************************************************************************/
STATUS NU_USB_STACK_Function_Suspend (  NU_USB_STACK *cb, 
                                        NU_USB_INTF *intf,
                                        BOOLEAN     func_suspend, 
                                        BOOLEAN     rmt_wakeup)
{
    STATUS status = NU_USB_INVLD_ARG;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();
    
    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(intf);

    if ( ((NU_USB_STACK_DISPATCH *)
            (((NU_USB *)cb)->usb_dispatch)) != NU_NULL)
    {
        if ( ((NU_USB_STACK_DISPATCH *)
            (((NU_USB *)cb)->usb_dispatch))->Function_Suspend
                                                         !=  NU_NULL)
        {
            /* Call the dispatch table function. */
            status = ((NU_USB_STACK_DISPATCH *)
              (((NU_USB *)cb)->usb_dispatch))->Function_Suspend (cb,
                            intf, func_suspend, rmt_wakeup);
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ( status );
}

#endif /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */

#endif /* USB_STACK_EXT_C */
/*************************** end of file ********************************/
